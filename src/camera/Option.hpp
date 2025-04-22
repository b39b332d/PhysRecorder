#pragma once
namespace capture {

    typedef enum {
        OPTION_INVALID = 0,
        OPTION_AUTO,
        OPTION_MANUAL
    } OPTION_TYPE;
    struct option_status {
        int value;
        OPTION_TYPE status_type;
    };
    struct option_range {
        bool is_supported;
        int min;
        int max;
        int step;
        double scaled_factor;
        OPTION_TYPE support_type;
        option_status def;
        option_status current;
    };

    class Options {
        int configuration_count = 0;
    protected:
        option_status empty_status = {0,};
        option_range* configurations;
        void set_option_range(int option, const option_range& option_range) {
            configurations[option] = option_range;
        }
    public:
        void* buffer=nullptr;
        int buffer_len = 0;
        Options(int configuration_count) : configuration_count(configuration_count) {
            configurations = new option_range[configuration_count](); // zero    initialized
        }
        ~Options() {
            delete[] configurations;
            free(buffer);
        }
        const option_range &get_option_range(int option) {
            return configurations[option];
        }
        const option_status &get_option(int option) {
            return configurations[option].current;
        }
        void set_value(int option,double value) {
            configurations[option].current.status_type = OPTION_MANUAL;
             configurations[option].current.value = value * configurations[option].scaled_factor;
        }
        double get_option_value(int option) {
            if (configurations[option].is_supported) {
                return (double)(configurations[option].current.value) / configurations[option].scaled_factor;
            }
            return 0;
        }
        void set_option_force(int option, const option_status& value) {
            configurations[option].current = value;
        }
        bool set_option(int option, const option_status& value) {
            if (configurations[option].is_supported) {
                set_option_native(option, value);
                return true;
            }
            return false;
        }
        const option_status &get_reset_option(int option) {
            return configurations[option].def;
        }
		bool is_default(int option) {
            return !configurations[option].is_supported || configurations[option].current.status_type == configurations[option].def.status_type &&
                 configurations[option].current.value == configurations[option].def.value;
		}
        bool is_supported(int option) {
            return configurations[option].is_supported;
        }
        virtual void set_option_native(int option, const option_status& value) {
			configurations[option].current = value;
        };
    };
}