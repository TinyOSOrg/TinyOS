#include <shared/sys.h>
#include <lib/assert.h>

void _lib_assert_impl(const char * file, const char *func, int line)
{
    printf("%LAssertion failed in %s, %s, %u\n", file, func, (uint32_t)line);
}
