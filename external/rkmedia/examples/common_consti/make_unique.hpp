//
// Created by consti10 on 17.10.21.
//

#ifndef RV1126_RV1109_V2_2_0_20210825_MAKE_UNIQUE_H
#define RV1126_RV1109_V2_2_0_20210825_MAKE_UNIQUE_H

// for some reason make_unique is not available to the compiler
namespace std{
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}

#endif //RV1126_RV1109_V2_2_0_20210825_MAKE_UNIQUE_H
