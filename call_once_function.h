#ifndef CALL_ONCE_FUNCTION_H
#define CALL_ONCE_FUNCTION_H

#include <bits/stdc++.h>

template<typename F>
struct call_once_function;



template<typename R, typename... As>
struct call_once_function<R(As...)> {

    constexpr static std::size_t small_storage_size = sizeof(void*);
    constexpr static std::size_t small_storage_alignment = alignof(void*);
    using small_storage_type = std::aligned_storage<small_storage_size, small_storage_alignment>::type;

    template<typename F>
    constexpr static bool fits_small_storage_v = sizeof(F) <= small_storage_size &&
                                                 alignof(F) <= small_storage_alignment &&
                                                 std::is_trivially_move_constructible_v<F>;

    template<typename F>
    constexpr static bool not_fits_small_storage_v = !fits_small_storage_v<F>;

    template<typename F>
    static std::enable_if_t<fits_small_storage_v<F>, F&> as(small_storage_type& storage) {
        return reinterpret_cast<F&>(storage);
    }

    template<typename F>
    static std::enable_if_t<not_fits_small_storage_v<F>, F&> as(small_storage_type& storage) {
        return *reinterpret_cast<F*&>(storage);
    }

    template<typename F>
    static std::enable_if_t<fits_small_storage_v<F>> move(small_storage_type& dst, small_storage_type& src) {
        new (&dst) F(std::move(as<F>(src)));
        as<F>(src).~F();
    }

    template<typename F>
    static std::enable_if_t<not_fits_small_storage_v<F>> move(small_storage_type& dst, small_storage_type& src) {
        reinterpret_cast<F*&>(dst) = reinterpret_cast<F*&>(src);
    }

    template<typename F>
    static std::enable_if_t<fits_small_storage_v<F>, R> call(small_storage_type& cur, As&&... args) {
        return std::move(as<F>(cur))(std::forward<As>(args)...);
    }

    template<typename F>
    static std::enable_if_t<not_fits_small_storage_v<F>, R> call(small_storage_type &cur, As&&... args) {
        return std::move(as<F>(cur))(std::forward<As>(args)...);
    }

    template<typename F>
    static std::enable_if_t<fits_small_storage_v<F>> destroy(small_storage_type& cur) {
        as<F>(cur).~F();
    }

    template<typename F>
    static std::enable_if_t<not_fits_small_storage_v<F>> destroy(small_storage_type& cur) {
        delete &as<F>(cur);
    }


    static void empty_move(small_storage_type&, small_storage_type&) {}
    static void empty_destroy(small_storage_type&) {}
    static R empty_call(small_storage_type&, As&&...) { throw std::bad_function_call(); }

    struct call_once_function_ops {
        using call_t  = R(*)(small_storage_type&, As&&...);
        using move_t = void(*)(small_storage_type&, small_storage_type&);
        using destroy_t = void(*)(small_storage_type&);

        call_once_function_ops(call_t call, move_t move, destroy_t destroy)
            : call(call), move(move), destroy(destroy) {}

        call_t call;
        move_t move;
        destroy_t destroy;
    };

    template<typename F>
    call_once_function_ops* make_function_ops() {
        static auto f = call_once_function_ops(&call<F>, &move<F>, &destroy<F>);
        return &f;
    }

    call_once_function_ops* make_empty_function_ops() {
        static auto f = call_once_function_ops(&empty_call, &empty_move, &empty_destroy);
        return &f;
    }

    call_once_function_ops* fs;
    small_storage_type storage;

    call_once_function() : fs(make_empty_function_ops()) {}

    call_once_function(std::nullptr_t) : fs(make_empty_function_ops()) {}

    call_once_function(call_once_function&& other) : fs(other.fs) {
       fs->move(storage, other.storage);
       fs->destroy(other.storage);
       other.fs = make_empty_function_ops();
    }

    template<typename F>
    call_once_function(F f) : fs(make_function_ops<F>()) {
        if (fits_small_storage_v<F>)
            new (&storage) F(std::move(f));
        else
            new (&storage) F*(new F(std::move(f)));
//        move<F>(storage, f.storage);
    }

    void swap(call_once_function &rhs) {
            std::swap(storage, rhs.storage);
            std::swap(fs, rhs.fs);
        }

    R operator () (As&&... args) {
        return fs->call(storage, std::forward<As>(args)...);
    }

};

#endif // CALL_ONCE_FUNCTION_H
