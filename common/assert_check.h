#ifndef HAJO_ASSERT_CHECK_H
#define HAJO_ASSERT_CHECK_H

#define ASSERT_CHECK(x) do {                                            \
        int result_ = (x);                                              \
        if (unlikely(result_ == 0)) {                                   \
            _esp_error_check_failed(result_, __FILE__, __LINE__,        \
            __ASSERT_FUNC, #x);                                         \
        }                                                               \
    } while(0)

#endif //HAJO_ASSERT_CHECK_H
