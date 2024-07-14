#include <tuple>
#include <initializer_list>

template <typename Key, typename Value>
using KvpInit = std::initializer_list<std::pair<Key,Value>>;

template <typename Key, typename Value>
constexpr auto generate_direct_heuro_(const KvpInit<Key,Value>& init);

template <typename Key, typename Value, typename Size, Size lst_size>
struct HeuroDirect {
    public:
        constexpr auto begin() {
            return std::begin(direct_data);
        }

        constexpr auto end() {
            return std::end(direct_data);
        }

        constexpr Value* data() {
            return &direct_data[0];
        }

        constexpr const Value& operator[](Key idx) const {
            return direct_data[idx];
        }

    private:
        constexpr HeuroDirect(const KvpInit<Key, Value> &lst) : direct_data() {
            static_assert(keys_starts_from_zero(lst), "Keys must start from zero!");
            static_assert(keys_no_gaps(lst), "Keys must not have gaps!");
            for (const auto &kvp : lst) {
                direct_data[kvp.first] = kvp.second;
            }
        }

      friend constexpr auto generate_direct_heuro_<Key, Value>(const KvpInit<Key,Value>& init);
      Value direct_data[lst_size];
};

template <typename Key, typename Value>
constexpr auto generate_direct_heuro_(const KvpInit<Key,Value>& init) {
    return HeuroDirect<Key,Value, std::size_t, 3>(init);
}

template <typename Key, typename Value>
static constexpr auto thing(const KvpInit<Key,Value> &init) {
    return generate_direct_heuro_<Key, Value>(init);
}