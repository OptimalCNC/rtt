# Invalid Struct Indexing Error Design

## Context

OCL accepts `value[index]` as valid expression syntax. Whether the value is
indexable is decided during semantic analysis. A MetaNC operation result such
as `SpindleProcessSnapshotBatch` is a struct containing a fixed C array; the
struct itself is not indexable.

`ExpressionParser::seen_index()` asks the value's type information for an
indexed member and already reports `Illegal use of []` when that lookup returns
null. `StructTypeInfo` instead asserts in its data-source identifier overload.
Consequently, invalid user input can abort assertion-enabled deployers before
the parser can report the semantic error.

## Required Behavior

- Direct indexing of a struct is rejected as a semantic error.
- Invalid TaskBrowser input never terminates the deployer.
- Named struct member access remains unchanged.
- Sequence and C-array indexing remain unchanged, including readonly indexing
  of arrays reached through operation results.
- Struct members do not acquire numeric or positional indexing semantics.

## Design

Change the unsupported `StructTypeInfo::getMember(item, id)` overload to
return a null data source instead of asserting. This follows the
`MemberFactory` failure contract and lets the existing expression parser emit
its normal `Illegal use of []` semantic error.

No parser-side type-name checks or new type capability API are needed. The
type-specific member factory remains responsible for accepting or rejecting
the lookup.

## Regression Coverage

Add a scripting regression using an operation that returns a struct containing
a C array:

- `getBatch()[0]` must raise a fatal semantic parse error containing
  `Illegal use of []`.
- The test process must remain alive after the rejection.
- Existing `getBatch().ints[0]` readonly access and readonly assignment
  rejection tests must continue to pass.

The pre-fix focused test is expected to abort at the `StructTypeInfo`
assertion. After the production change, run the focused scripting and type
discovery suites, followed by the full RTT test suite and the exact MetaNC
TaskBrowser expression.

## Scope

This is an RTT error-handling correction. It does not change MetaNC data types,
add shorthand indexing for wrapper structs, or alter the readonly C-array
implementation.
