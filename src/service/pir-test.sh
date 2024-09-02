#!/bin/bash

# bazel test --test_output=streamed  --nocache_test_results //ta:pir_multi_query --test_filter=PirMultiQueryTest.test100_1
# bazel test --test_output=streamed  --nocache_test_results //ta:pir_multi_query --test_filter=PirMultiQueryTest.test1000_1
# bazel test --test_output=streamed  --nocache_test_results //ta:pir_multi_query --test_filter=PirMultiQueryTest.test1w_1
# bazel test --test_output=streamed  --nocache_test_results //ta:pir_multi_query --test_filter=PirMultiQueryTest.test10w_1
bazel test --test_output=streamed  --nocache_test_results //ta:pir_multi_query --test_filter=PirMultiQueryTest.test100w_1
# bazel test --test_output=streamed  --nocache_test_results //ta:pir_multi_query --test_filter=PirMultiQueryTest.test1000w_1
# bazel test --test_output=streamed  --nocache_test_results //ta:pir_multi_query --test_filter=PirMultiQueryTest.test10000w_1
