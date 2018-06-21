#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <kitchensink/kitchensink.h>

Kit_Source *src = NULL;

void test_Kit_CreateSourceFromUrl(void) {
    CU_ASSERT_PTR_NULL(Kit_CreateSourceFromUrl("nonexistent"));
    src = Kit_CreateSourceFromUrl("../../tests/data/CEP140_512kb.mp4");
    CU_ASSERT_PTR_NOT_NULL(src);
}

void test_Kit_GetBestSourceStream(void) {
    CU_ASSERT(Kit_GetBestSourceStream(src, KIT_STREAMTYPE_VIDEO) == 0);
    CU_ASSERT(Kit_GetBestSourceStream(src, KIT_STREAMTYPE_AUDIO) == 1);
    CU_ASSERT(Kit_GetBestSourceStream(src, KIT_STREAMTYPE_UNKNOWN) == -1);
    CU_ASSERT(Kit_GetBestSourceStream(src, KIT_STREAMTYPE_DATA) == -1);
    CU_ASSERT(Kit_GetBestSourceStream(src, KIT_STREAMTYPE_ATTACHMENT) == -1);
    CU_ASSERT(Kit_GetBestSourceStream(src, KIT_STREAMTYPE_SUBTITLE) == -1);
}

void test_Kit_GetSourceStreamCount(void) {
    CU_ASSERT(Kit_GetSourceStreamCount(src) == 2);
}

void test_Kit_SetSourceStream(void) {
    CU_ASSERT(Kit_SetSourceStream(src, KIT_STREAMTYPE_VIDEO, 0) == 0);
    CU_ASSERT(Kit_SetSourceStream(src, KIT_STREAMTYPE_UNKNOWN, 0) == 1);
}

void test_Kit_GetSourceStream(void) {
    CU_ASSERT(Kit_GetSourceStream(src, KIT_STREAMTYPE_VIDEO) == 0);
    CU_ASSERT(Kit_GetSourceStream(src, KIT_STREAMTYPE_AUDIO) == 1);
    CU_ASSERT(Kit_GetSourceStream(src, KIT_STREAMTYPE_UNKNOWN) == -1);
}

void test_Kit_CloseSource(void) {
    Kit_CloseSource(src);
}

void source_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "Kit_CreateSourceFromUrl", test_Kit_CreateSourceFromUrl) == NULL) { return; }
    if(CU_add_test(suite, "Kit_GetBestSourceStream", test_Kit_GetBestSourceStream) == NULL) { return; }
    if(CU_add_test(suite, "Kit_GetSourceStreamCount", test_Kit_GetSourceStreamCount) == NULL) { return; }
    if(CU_add_test(suite, "Kit_SetSourceStream", test_Kit_SetSourceStream) == NULL) { return; }
    if(CU_add_test(suite, "Kit_GetSourceStream", test_Kit_GetSourceStream) == NULL) { return; }
    if(CU_add_test(suite, "Kit_CloseSource", test_Kit_CloseSource) == NULL) { return; }
}
