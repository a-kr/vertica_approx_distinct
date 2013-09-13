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
#define V_32K_AND_A_BIT 33000
#define HLL_BITS 13
#define LPC_BITS (63 * 1024 * 8)
#define ESTIMATOR_ARG HLL_BITS
#define EstimatorClass HyperLogLogOwnArrayCounter

class EstimateCountDistinct : public AggregateFunction
{

    public:

    void serialize_counter(ICardinalityEstimator *counter, IntermediateAggs &aggs) {
        Serializer ser;
        ser.add_storage(aggs.getStringRef(1).data(), VARBINARY_MAX);
        //ser.add_storage(aggs.getStringRef(2).data(), VARBINARY_MAX);
        //ser.add_storage(aggs.getStringRef(3).data(), VARBINARY_MAX);
        counter->serialize(&ser);
    }

    void unserialize_counter(ICardinalityEstimator *counter, IntermediateAggs &aggs) {
        Serializer ser;
        ser.add_storage(aggs.getStringRef(1).data(), VARBINARY_MAX);
        //ser.add_storage(aggs.getStringRef(2).data(), VARBINARY_MAX);
        //ser.add_storage(aggs.getStringRef(3).data(), VARBINARY_MAX);
        counter->unserialize(&ser);
    }

    void unserialize_counter(ICardinalityEstimator *counter, MultipleIntermediateAggs &aggs) {
        Serializer ser;
        ser.add_storage((char *)aggs.getStringRef(1).data(), VARBINARY_MAX);
        //ser.add_storage((char *)aggs.getStringRef(2).data(), VARBINARY_MAX);
        //ser.add_storage((char *)aggs.getStringRef(3).data(), VARBINARY_MAX);
        counter->unserialize(&ser);
    }

    virtual void initAggregate(ServerInterface &srvInterface, IntermediateAggs &aggs)
    {
        try {
            srvInterface.log("aggregate init");
            vint &estimator_arg = aggs.getIntRef(0);
            estimator_arg = ESTIMATOR_ARG;
            aggs.getStringRef(1).copy(std::string((size_t)V_32K_AND_A_BIT, '\0'));
            aggs.getStringRef(2).copy(std::string((size_t)V_32K_AND_A_BIT, '\0'));
            EstimatorClass counter(estimator_arg, aggs.getStringRef(1).data(), aggs.getStringRef(2).data());
            //aggs.getStringRef(2).copy(std::string((size_t)VARBINARY_MAX, ' '));
            //aggs.getStringRef(3).copy(std::string((size_t)VARBINARY_MAX, ' '));
            //this->serialize_counter(&counter, aggs);
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
            vint estimator_arg = aggs.getIntRef(0);
            //EstimatorClass counter(estimator_arg);
            //EstimatorClass counter(estimator_arg, aggs.getStringRef(1).data());
            EstimatorClass counter(estimator_arg, aggs.getStringRef(1).data(), aggs.getStringRef(2).data());
            //this->unserialize_counter(&counter, aggs);

            do {
                const VString &input = argReader.getStringRef(0);
                counter.increment(input.data(), input.length());
            } while (argReader.next());

            //this->serialize_counter(&counter, aggs);
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
            vint estimator_arg = aggs.getIntRef(0);
            //EstimatorClass counter(estimator_arg, aggs.getStringRef(1).data());
            EstimatorClass counter(estimator_arg, aggs.getStringRef(1).data(), aggs.getStringRef(2).data());
            //EstimatorClass counter(estimator_arg);
            //this->unserialize_counter(&counter, aggs);

            do {
                //EstimatorClass other_counter(estimator_arg);
                //EstimatorClass other_counter(estimator_arg, (char *)aggsOther.getStringRef(1).data());
                EstimatorClass other_counter(estimator_arg, (char *)aggsOther.getStringRef(1).data(), (char *)aggsOther.getStringRef(2).data());
                //this->unserialize_counter(&other_counter, aggsOther);
                counter.merge_from(&other_counter);
            } while (aggsOther.next());

            //this->serialize_counter(&counter, aggs);
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
            vint estimator_arg = aggs.getIntRef(0);
            //EstimatorClass counter(estimator_arg, aggs.getStringRef(1).data());
            EstimatorClass counter(estimator_arg, (char *)aggs.getStringRef(1).data(), (char *)aggs.getStringRef(2).data());
            //EstimatorClass counter(estimator_arg);
            //this->unserialize_counter(&counter, aggs);

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
        intermediateTypeMetaData.addVarbinary(V_32K_AND_A_BIT, "storage0");
        intermediateTypeMetaData.addVarbinary(V_32K_AND_A_BIT, "storage1");
        //intermediateTypeMetaData.addVarbinary(VARBINARY_MAX, "storage2");
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

