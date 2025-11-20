#ifndef RA_CPP_UNIT_TEST_HH
#define RA_CPP_UNIT_TEST_HH

/* completely suppress everything inside the CppUnitTest.h file */
#include <CodeAnalysis\Warnings.h>
#pragma warning(push)
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS ALL_CODE_ANALYSIS_WARNINGS)
#include <CppUnitTest.h>
#pragma warning(pop)

/* This has to be globally disabled in the unit tests because it occurs in the
 * macros provided by the framework (i.e. TEST_METHOD).
 */
#pragma warning(disable: 26477) /* Use 'nullptr' rather than 0 or NULL (es.47). */

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#endif RA_CPP_UNIT_TEST_HH
