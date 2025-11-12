#include "./tmp.cpp"

template <class... Sensors>
class SensorManager {
  public:
    SensorManager(Sensors*... sensors) : sensors_(sensors...) {
      sensors_init()
    }
    
    void init() {
      std::for_each(sensors, [](auto const& s) { s->init(); });
    }
    
    static size_t NUM = sizeof...(Sensors);
    // largest data packet?
    // list of sensors we have
  
    void sensors_init() {
      for(size_t i = 0; i < NUM; i++) {
        
      }
    }

    std::vector<SensorDataDescriptor<idk>> dataDescriptor
};

