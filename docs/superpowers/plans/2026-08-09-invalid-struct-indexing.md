# Invalid Struct Indexing Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reject direct indexing of RTT struct values as an OCL semantic error without terminating an assertion-enabled deployer.

**Architecture:** Keep indexability owned by each type's `MemberFactory`. Make the unsupported dynamic-member overload in `StructTypeInfo` return null, allowing the existing `ExpressionParser::seen_index()` failure path to emit `Illegal use of []`; do not add parser type-name checks or positional struct indexing.

**Tech Stack:** C++17, Orocos RTT type information and scripting parser, Boost.Test, CMake/CTest, OCL TaskBrowser.

## Global Constraints

- Direct indexing of a struct is rejected as a semantic error.
- Invalid TaskBrowser input never terminates the deployer.
- Named struct member access remains unchanged.
- Sequence and C-array indexing remain unchanged, including readonly indexing of arrays reached through operation results.
- Struct members do not acquire numeric or positional indexing semantics.
- Keep the production change limited to the unsupported `StructTypeInfo::getMember(item, id)` overload.

---

## File Structure

- `tests/scripting_test.cpp`: exercises OCL expressions against a real operation result and proves invalid direct struct indexing is rejected while valid member-array indexing still works.
- `rtt/types/StructTypeInfo.hpp`: implements named struct member discovery and rejects unsupported dynamic member identifiers through the `MemberFactory` null-result contract.

### Task 1: Reject Dynamic Struct Indexes Without Aborting

**Files:**
- Modify: `tests/scripting_test.cpp:439`
- Modify: `rtt/types/StructTypeInfo.hpp:91`

**Interfaces:**
- Consumes: `Parser::parseExpression(const std::string&, TaskContext*)`, `ReadOnlyCArrayOperationProvider::getBatch()`, and `MemberFactory::getMember(item, id)`.
- Produces: `StructTypeInfo<T>::getMember(item, id)` returns a null `DataSourceBase::shared_ptr` for unsupported dynamic identifiers; `ExpressionParser::seen_index()` converts that null result into `parse_exception_fatal_semantic_error`.

- [ ] **Step 1: Add the failing scripting regression**

Insert this test after `TestCallResultCArrayIndexingIsReadOnly`:

```cpp
BOOST_AUTO_TEST_CASE(TestDirectStructIndexingIsRejectedWithoutAborting)
{
    if (!Types()->type("cints")) {
        Types()->addType(new CArrayTypeInfo<carray<int> >("cints"));
    }
    if (!Types()->type("BType")) {
        Types()->addType(new StructTypeInfo<BType>("BType"));
    }

    ReadOnlyCArrayOperationProvider provider;
    tc->provides("test")->addOperation(
        "getBatch", &ReadOnlyCArrayOperationProvider::getBatch, &provider);

    Parser parser(caller->engine());
    try {
        parser.parseExpression("test.getBatch()[0]", tc);
        BOOST_FAIL("Direct struct indexing was accepted");
    } catch (const parse_exception_fatal_semantic_error& error) {
        BOOST_CHECK_NE(
            std::string(error.what()).find("Illegal use of []"),
            std::string::npos);
    }

    DataSourceBase::shared_ptr valid =
        parser.parseExpression("test.getBatch().ai[3]", tc);
    BOOST_REQUIRE(valid);
    DataSource<int>::shared_ptr value = DataSource<int>::narrow(valid.get());
    BOOST_REQUIRE(value);
    BOOST_CHECK_EQUAL(value->get(), 99);
}
```

- [ ] **Step 2: Build and run the new test to verify RED**

Run:

```bash
cmake --build build --target scripting_test -j2
build/tests/scripting_test \
  --run_test=ScriptingTestSuite/TestDirectStructIndexingIsRejectedWithoutAborting \
  --log_level=test_suite
```

Expected: the test process exits nonzero at the `StructTypeInfo.hpp` assertion. This is the required RED signal: the invalid expression aborts before the test can catch a `parse_exception`.

- [ ] **Step 3: Implement the minimal type-info correction**

Replace the asserting overload in `StructTypeInfo` with:

```cpp
virtual base::DataSourceBase::shared_ptr getMember(
        base::DataSourceBase::shared_ptr,
        base::DataSourceBase::shared_ptr) const {
    // Dynamic identifiers are supported only by sequence-like types.
    return base::DataSourceBase::shared_ptr();
}
```

Do not change `ExpressionParser`, `CArrayTypeInfo`, or named struct member discovery.

- [ ] **Step 4: Rebuild and run the focused GREEN regressions**

Run:

```bash
cmake --build build --target scripting_test type_discovery_struct_test -j2
build/tests/scripting_test \
  --run_test=ScriptingTestSuite/TestDirectStructIndexingIsRejectedWithoutAborting:ScriptingTestSuite/TestCallResultCArrayIndexingIsReadOnly:ScriptingTestSuite/TestCallResultIndexing \
  --log_level=test_suite
build/tests/type_discovery_struct_test \
  --run_test=TypeArchiveTestSuite/testReadOnlyCArrayElementsAreReadable:TypeArchiveTestSuite/testReadOnlyCArrayElementCopyUsesReplacementParent \
  --log_level=test_suite
```

Expected: all five selected test cases pass. The invalid expression is caught as `Illegal use of []`, valid readonly C-array reads still return `99`, and returned sequence indexing remains supported.

- [ ] **Step 5: Run the complete RTT suite and inspect every failure**

Run:

```bash
ctest --test-dir build --output-on-failure
```

Expected: no new failures. `property_loader_test` is the previously recorded unrelated baseline failure; if it still fails, confirm it is the sole failure and retain its output for the final report. Any other failure blocks completion.

- [ ] **Step 6: Commit the regression and production correction**

Run:

```bash
git add tests/scripting_test.cpp rtt/types/StructTypeInfo.hpp
git diff --cached --check
git commit -m "fix: reject invalid struct indexing safely"
```

### Task 2: Verify the Exact MetaNC TaskBrowser Behavior

**Files:**
- No source changes.
- Runtime inputs: `/tmp/orocos-carray-worktree-install`, `/tmp/metanc-carray-deployer.tcQ4VX/metanc/runtime-build`, and `/tmp/metanc-carray-deployer.tcQ4VX/run-deployer.sh`.

**Interfaces:**
- Consumes: the installed RTT headers from Task 1 and the isolated MetaNC typekit/component build.
- Produces: live evidence that invalid direct struct indexing reports a semantic error and the same TaskBrowser process continues to evaluate valid member-array indexing.

- [ ] **Step 1: Install the corrected RTT header into the isolated prefix**

Run:

```bash
cmake --install build
```

Expected: `/tmp/orocos-carray-worktree-install/include/orocos/rtt/types/StructTypeInfo.hpp` contains the null-return overload without the unsupported-index assertion.

- [ ] **Step 2: Rebuild and install the temporary MetaNC runtime**

Run:

```bash
cmake --build /tmp/metanc-carray-deployer.tcQ4VX/metanc/runtime-build \
  --target install -j2
```

Expected: the MetaNC typekits and components rebuild successfully against the corrected installed RTT header.

- [ ] **Step 3: Exercise invalid and valid expressions in one PTY session**

Start `/tmp/metanc-carray-deployer.tcQ4VX/run-deployer.sh` in a PTY. At `Deployer [S]>`, enter these lines in order:

```text
rt_api.motion_bank.spindles.process_data_out.last()[0]
rt_api.motion_bank.spindles.process_data_out.last().spindles[0]
rt_api.motion_bank.spindles.process_data_out.last().spindles[0].valid = false
quit
```

Expected:

- The first expression reports `Illegal use of []` and returns to `Deployer [S]>`.
- The second expression prints spindle entry `0` with `valid: true`.
- The assignment is rejected as a constant or returned variable.
- `quit` exits with status `0`, and no deployer process remains.

- [ ] **Step 4: Confirm source worktree boundaries**

Run:

```bash
git status --short --branch
git -C /home/liufang/MetaNC/ws/src/MetaNC status --short --branch
```

Expected: the RTT feature worktree contains only planned commits and no uncommitted files; the active MetaNC checkout remains unchanged.
