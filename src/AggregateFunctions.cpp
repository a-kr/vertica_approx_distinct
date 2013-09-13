/* Copyright (c) 2005 - 2012 Vertica, an HP company -*- C++ -*- */
/* 
 * Description: Example User Defined Aggregate Function: Estimate COUNT(DISTINCT x)
 *
 */

#include "Vertica.h"
#include <time.h>
#include <sstream>
#include <vector>
#include <iostream>
#include "CardinalityEstimators.h"

using namespace Vertica;
using namespace std;

#define VARBINARY_MAX 65000
#define HLL_BITS 15
#define LPC_BITS (128 * 1024 * 8)
#define ESTIMATOR_ARG HLL_BITS
#define EstimatorClass HyperLogLogCounter

class EstimateCountDistinct : public AggregateFunction
{

    public:

    void serialize_counter(ICardinalityEstimator *counter, IntermediateAggs &aggs) {
        Serializer ser;
        ser.add_storage(aggs.getStringRef(1).data(), VARBINARY_MAX);
        ser.add_storage(aggs.getStringRef(2).data(), VARBINARY_MAX);
        ser.add_storage(aggs.getStringRef(3).data(), VARBINARY_MAX);
        counter->serialize(&ser);
    }

    void unserialize_counter(ICardinalityEstimator *counter, IntermediateAggs &aggs) {
        Serializer ser;
        ser.add_storage(aggs.getStringRef(1).data(), VARBINARY_MAX);
        ser.add_storage(aggs.getStringRef(2).data(), VARBINARY_MAX);
        ser.add_storage(aggs.getStringRef(3).data(), VARBINARY_MAX);
        counter->unserialize(&ser);
    }

    void unserialize_counter(ICardinalityEstimator *counter, MultipleIntermediateAggs &aggs) {
        Serializer ser;
        ser.add_storage((char *)aggs.getStringRef(1).data(), VARBINARY_MAX);
        ser.add_storage((char *)aggs.getStringRef(2).data(), VARBINARY_MAX);
        ser.add_storage((char *)aggs.getStringRef(3).data(), VARBINARY_MAX);
        counter->unserialize(&ser);
    }

    virtual void initAggregate(ServerInterface &srvInterface, IntermediateAggs &aggs)
    {
        try {
            srvInterface.log("aggregate init");
            vint &estimator_arg = aggs.getIntRef(0);
            estimator_arg = ESTIMATOR_ARG;
            EstimatorClass counter(estimator_arg);
            aggs.getStringRef(1).copy(std::string((size_t)VARBINARY_MAX, ' '));
            aggs.getStringRef(2).copy(std::string((size_t)VARBINARY_MAX, ' '));
            aggs.getStringRef(3).copy(std::string((size_t)VARBINARY_MAX, ' '));
            this->serialize_counter(&counter, aggs);
        } catch(exception& e) {
            // Standard exception. Quit.
            vt_report_error(0, "Exception while initializing intermediate aggregates: [%s]", e.what());
        }
    }

    void aggregate(ServerInterface &srvInterface,
                   BlockReader &argReader,
                   IntermediateAggs &aggs)
    {
        try {
            srvInterface.log("aggregate aggregate");
            vint estimator_arg = aggs.getIntRef(0);
            EstimatorClass counter(estimator_arg);
            this->unserialize_counter(&counter, aggs);

            do {
                const VString &input = argReader.getStringRef(0);
                counter.increment(input.str().c_str());
            } while (argReader.next());

            this->serialize_counter(&counter, aggs);
        } catch(exception& e) {
            // Standard exception. Quit.
            vt_report_error(0, "Exception while processing aggregate: [%s]", e.what());
        }
    }

    virtual void combine(ServerInterface &srvInterface,
                         IntermediateAggs &aggs,
                         MultipleIntermediateAggs &aggsOther)
    {
        try {
            srvInterface.log("aggregate combine");
            vint estimator_arg = aggs.getIntRef(0);
            EstimatorClass counter(estimator_arg);
            this->unserialize_counter(&counter, aggs);

            do {
                EstimatorClass other_counter(estimator_arg);
                this->unserialize_counter(&counter, aggsOther);
                counter.merge_from(&other_counter);
            } while (aggsOther.next());

            this->serialize_counter(&counter, aggs);
        } catch(exception& e) {
            // Standard exception. Quit.
            vt_report_error(0, "Exception while combining intermediate aggregates: [%s]", e.what());
        }
    }

    virtual void terminate(ServerInterface &srvInterface,
                           BlockWriter &resWriter,
                           IntermediateAggs &aggs)
    {
        try {
            srvInterface.log("aggregate terminate");
            vint estimator_arg = aggs.getIntRef(0);
            EstimatorClass counter(estimator_arg);
            this->unserialize_counter(&counter, aggs);

            int count = counter.count();
            resWriter.setInt(count);
        } catch(exception& e) {
            // Standard exception. Quit.
            vt_report_error(0, "Exception while computing aggregate output: [%s]", e.what());
        }
    }

    InlineAggregate()
};


class EstimateCountDistinctFactory : public AggregateFunctionFactory
{
    virtual void getIntermediateTypes(ServerInterface &srvInterface, const SizedColumnTypes &inputTypes, SizedColumnTypes &intermediateTypeMetaData)
    {
        intermediateTypeMetaData.addInt("b");
        intermediateTypeMetaData.addVarbinary(VARBINARY_MAX, "storage0");
        intermediateTypeMetaData.addVarbinary(VARBINARY_MAX, "storage1");
        intermediateTypeMetaData.addVarbinary(VARBINARY_MAX, "storage2");
    }

    virtual void getPrototype(ServerInterface &srvfloaterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addVarchar();
        returnType.addInt();
    }

    virtual void getReturnType(ServerInterface &srvfloaterface,
                               const SizedColumnTypes &inputTypes,
                               SizedColumnTypes &outputTypes)
    {
        outputTypes.addInt("est_count");
    }

    virtual AggregateFunction *createAggregateFunction(ServerInterface &srvfloaterface)
    { return vt_createFuncObj(srvfloaterface.allocator, EstimateCountDistinct); }

};

RegisterFactory(EstimateCountDistinctFactory);

