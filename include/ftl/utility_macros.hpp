#ifndef FTL_UTILITY_HPP
#define FTL_UTILITY_HPP

#define FTL_MOVE(...) \
    static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)

#define FTL_FORWARD(...) \
    static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

#endif
