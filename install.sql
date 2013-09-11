\set libfile '\''`pwd`'/build/AggregateFunctions.so\'';
CREATE LIBRARY MyAggregateFunctions AS :libfile;

-- Step 2: Create Functions
CREATE AGGREGATE FUNCTION estimate_count_distinct AS LANGUAGE 'C++'
NAME 'EstimateCountDistinctFactory' LIBRARY MyAggregateFunctions;

