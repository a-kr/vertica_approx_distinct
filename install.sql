\set libfile '\''`pwd`'/build/CardinalityEstimators.so\'';
CREATE LIBRARY CardinalityEstimators AS :libfile;

-- Step 2: Create Functions
CREATE AGGREGATE FUNCTION estimate_count_distinct AS LANGUAGE 'C++'
NAME 'EstimateCountDistinctFactory' LIBRARY CardinalityEstimators;

