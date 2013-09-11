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
#include "LinearProbCounter.h"

using namespace Vertica;
using namespace std;


class EstimateCountDistinct : public AggregateFunction
{

    std::vector<int> counters;

    public:
    EstimateCountDistinct(): counters()
    {
    }

    virtual void initAggregate(ServerInterface &srvInterface, IntermediateAggs &aggs)
    {
        try {
            vint &my_state = aggs.getIntRef(0);
            this->counters.push_back(0);
            my_state = this->counters.size() - 1;
            my_state = 0;
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
            vint &my_state = aggs.getIntRef(0);
            do {
                //this->cocounter_iunters[counter_i] += 1;
                my_state += 1;
                //const VString &input = argReader.getStringRef(0);

                //if (!input.isNull()) {
                    //if ((word.length() + input.length()) > maxSize) break;
                    //word.append(input.str());
                //}
            } while (argReader.next());
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
            vint &my_counter_i = aggs.getIntRef(0);

            do {
                const vint &other_counter_i = aggsOther.getIntRef(0);
                //this->counters[my_counter_i] += this->counters[other_counter_i];
                //this->counters[other_counter_i] = 0;
                my_counter_i += other_counter_i;
            } while (aggsOther.next());
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
            vint &my_result = aggs.getIntRef(0);
            resWriter.setInt(my_result);
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
        intermediateTypeMetaData.addInt("est_count");
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

