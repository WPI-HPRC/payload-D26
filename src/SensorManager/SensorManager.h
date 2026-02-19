#include <tuple>
#include <cstdint>

// simple tuple_for_each helper so you don't have to read std::apply
template <typename Tuple, typename Func, std::size_t... I>
void tuple_for_each_impl(Tuple& t, Func f, std::index_sequence<I...>) {
    (f(std::get<I>(t)), ...);
}

template <typename Tuple, typename Func>
void tuple_for_each(Tuple& t, Func f) {
    constexpr std::size_t N =
        std::tuple_size<std::remove_reference_t<Tuple>>::value;
    tuple_for_each_impl(t, f, std::make_index_sequence<N>{});
}

 //MillisFn is a function that returns a uint32_t and is used to pass around millis()
 // the compiler makes it now = millis() in the end instead of millis_()
template <typename MillisFn, typename... Sensors>
class SensorManager {
public:
    SensorManager(MillisFn millis, Sensors&... sensors)
        : millis_(millis), sensors_(sensors...) {}

    bool sensorInit() {
        bool ok = true;
        tuple_for_each(sensors_, [&](auto& s) {
            ok = ok && s.init();
        });
        sensorCount_ = std::tuple_size<decltype(sensors_)>::value;
        return ok;
    }

    void loop() {
        const uint32_t now = millis_();
        tuple_for_each(sensors_, [&](auto& s) {
            if (s.getInitStatus() &&
                now - s.getLastTimePolled() >= s.getPollingPeriod()) {
                s.poll(now);
            }
        });
    }

    //TODO: fix size_t to the proper type
    size_t count() const {
        return sensorCount_;
    }

private:
    MillisFn millis_;
    std::tuple<Sensors&...> sensors_;
    size_t sensorCount_;
};
