#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "kitchensink/kitchensink.h"

void source_test_suite(CU_pSuite suite);

int main(int argc, char **argv) {
    CU_pSuite suite = NULL;

    Kit_Init(KIT_INIT_NETWORK|KIT_INIT_FORMATS);

    if(CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    }

    suite = CU_add_suite("Source functions", NULL, NULL);
    if(suite == NULL) goto end;
    source_test_suite(suite);

    // Run tests
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

end:
    CU_cleanup_registry();
    Kit_Quit();
    return CU_get_error();
}
