#ifndef CALL_ONCE_FUNCTION_H
#define CALL_ONCE_FUNCTION_H

#include <bits/stdc++.h>

template<typename F>
struct call_once_function;



template<typename R, typename... As>
struct call_once_function<R(As...)> {

    constexpr static std::size_t storage_size = sizeof(void*);
    constexpr static std::size_t storage_alignment = alignof(void*);
    using storage_type = std::aligned_storage<storage_size, storage_alignment>::type;

    template<typename F>
    constexpr static bool ok_for_fits_small_storage_v = sizeof(F) <= storage_size &&
                                                 alignof(F) <= storage_alignment &&
                                                 std::is_nothrow_move_constructible<F>();

    template<typename F>
    constexpr static bool not_ok_for_fits_small_storage_v = !ok_for_fits_small_storage_v<F>;

    template<typename F>
    static std::enable_if_t<ok_for_fits_small_storage_v<F>, F&> as(storage_type& storage) {
        return reinterpret_cast<F&>(storage);
    }

    template<typename F>
    static std::enable_if_t<not_ok_for_fits_small_storage_v<F>, F*&> as(storage_type& storage) {
        return reinterpret_cast<F*&>(storage);
    }

    template<typename F>
    static std::enable_if_t<ok_for_fits_small_storage_v<F>> move(storage_type& dst, storage_type& src) {
        new (&dst) F(std::move(as<F>(src)));
        as<F>(src).~F();
    }

    template<typename F>
    static std::enable_if_t<not_ok_for_fits_small_storage_v<F>> move(storage_type& dst, storage_type& src) {
        new (&dst) F*(std::move(as<F>(src)));
        as<F>(src) = nullptr;
    }

    template<typename F>
    static std::enable_if_t<ok_for_fits_small_storage_v<F>, R> call(storage_type& cur, As&&... args) {
        return std::move(as<F>(cur))(std::forward<As>(args)...);
    }

    template<typename F>
    static std::enable_if_t<not_ok_for_fits_small_storage_v<F>, R> call(storage_type &cur, As&&... args) {
        return std::move(*as<F>(cur))(std::forward<As>(args)...);
    }

    template<typename F>
    static std::enable_if_t<ok_for_fits_small_storage_v<F>> destroy(storage_type& cur) {
        as<F>(cur).~F();
    }

    template<typename F>
    static std::enable_if_t<not_ok_for_fits_small_storage_v<F>> destroy(storage_type& cur) {
        delete as<F>(cur);
    }


    static void empty_move(storage_type&, storage_type&) {}
    static void empty_destroy(storage_type&) {}
    static R empty_call(storage_type&, As&&...) { throw std::bad_function_call(); }

    struct call_once_function_ops {
        using call_t  = R(*)(storage_type&, As&&...);
        using move_t = void(*)(storage_type&, storage_type&);
        using destroy_t = void(*)(storage_type&);

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
    storage_type storage;

    call_once_function() : fs(make_empty_function_ops()) {}

    call_once_function(std::nullptr_t) : fs(make_empty_function_ops()) {}

    call_once_function(call_once_function&& other) : fs(other.fs) {
       fs->move(storage, other.storage);
       fs->destroy(other.storage);
       other.fs = make_empty_function_ops();
    }

    template<typename F, std::enable_if_t<ok_for_fits_small_storage_v<F>>* = nullptr>
    call_once_function(F f) : fs(make_function_ops<F>()) {
        new (&storage) F(std::move(f));
    }

    template<typename F, std::enable_if_t<not_ok_for_fits_small_storage_v<F>>* = nullptr>
    call_once_function(F f) : fs(make_function_ops<F>()) {
        new (&storage) F*(new F(std::move(f)));
    }

    void swap(call_once_function &other) noexcept {
        storage_type temp1, temp2;
        fs->move(temp1, storage);
        fs->destroy(storage);
        other.fs->move(temp2, other.storage);
        other.fs->destroy(other.storage);
        fs->move(other.storage, temp1);
        fs->destroy(temp1);
        other.fs->move(storage, temp2);
        other.fs->destroy(temp2);
        std::swap(fs, other.fs);
    }

    call_once_function& operator=(call_once_function&& other) noexcept {
        swap(other);
        return *this;
    }


    R operator () (As&&... args) {
        return fs->call(storage, std::forward<As>(args)...);
    }

    ~call_once_function() {
        fs->destroy(storage);
    }
};

#endif // CALL_ONCE_FUNCTION_H
