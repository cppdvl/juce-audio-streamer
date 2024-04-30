//
// Created by Julian Guarin on 19/12/23.
//

#ifndef WSCONNECT_SIGNALSSLOTS_H
#define WSCONNECT_SIGNALSSLOTS_H

#include <utility>
#include <functional>
#include <type_traits>
#include <unordered_map>
namespace DAWn::Events
{
    template <typename T, typename IDType = int32_t>
    class Id
    {
     public:
        Id() : mId(static_cast<IDType>(++sId)) {}
        ~Id() = default;
        const int GetId() const { return mId; }
     protected:
        IDType mId{0};

     private:
        inline static int32_t sId{0};

    };


    //template <typename... Args>
    //class Slot : public std::function<void(Args...)>, public Id<Slot<Args...>, SlotId> {};
    // Type trait to check if a member function pointer is callable with Args...
    template <typename T, typename... Args>
    struct is_callable_with : std::false_type {};

    template <typename T, typename... Args, typename... Params>
    struct is_callable_with<T (Params...), Args...> : std::integral_constant<bool, sizeof...(Args) == sizeof...(Params)> {};

    template<typename... Args>
    class Signal : public Id<Signal<Args...>>
    {

    public:
        using SlotId    = size_t;
        using Slot      = std::function<void(Args...)>;

        /*!
         * @brief Default constructor.
         */
        Signal() = default;
        ~Signal() = default;

        /*!
         * @brief Copy constructor. Deleted, because we don't want to copy the signal.
         */
        Signal(const Signal &) = delete;
        Signal &operator=(const Signal &) = delete;

        /*!
         * @brief Move constructor. Deleted, because we don't want to move the signal.
         */
        Signal(Signal &&) = delete;
        Signal &operator=(Signal &&) = delete;

        /*!
         * @brief Authenticate a slot to the signal.
         * @param slot. The slot to connect to the signal.
         * @return The slot id.

         * @tparam T class type you want to connect.
         * @param inst pointer to instance to the owner class of the slot you want to connect.
         * @param func pointer to the function (method) you want to connect.
         * @return SlotId of the slot you just connected.
         */

        /*template <typename T>
        SlotId Connect(T *inst, void (T::*func)() const) {
            return Connect([=]() { (inst->*func)(); });
        }*/

        /*template <typename T>
        SlotId Connect(T *inst, void (T::*func)(Args...))
        {
            return Connect([=](Args... args) { inst->*func(args...); });
        }*/

        /*template <typename T>
        SlotId Connect(
        T *inst,
        void (T::*func)(Args...)) {
            return Connect([=](Args... args) { (inst->*func)(args...); });
        }*/

        // Overload for member functions with parameters
        template <typename T>
        SlotId Connect(T *inst, void (T::*func)()) {
            // Handle no-argument member functions
            return Connect([=]() { (inst->*func)(); });
        }

        template <typename T, typename Arg1>
        SlotId Connect(T *inst, void (T::*func)(Arg1)) {
            // Handle single-argument member functions
            return Connect([=](Arg1 arg1) { (inst->*func)(arg1); });
        }

        template <typename T>
        SlotId Connect(T *inst, void (T::*func)() const) {
            // Handle no-argument member functions
            return Connect([=]() { (inst->*func)(); });
        }

        template <typename T, typename Arg1>
        SlotId Connect(T *inst, void (T::*func)(Arg1) const) {
            // Handle single-argument member functions
            return Connect([=](Arg1 arg1) { (inst->*func)(arg1); });
        }

        /*template <typename T, typename Arg1, typename Arg2>
        SlotId Connect(T *inst, void (T::*func)(Arg1, Arg2)) {
            // Handle two-argument member functions
        }*/

        /*!
         * @brief Authenticate a slot to the signal.
         * @param func. The function to connect to the signal.
         * @return The slot id.
         */
        SlotId Connect(void (*func)(Args...)){
            return Connect([=](Args... args) { (*func)(args...); });
        }

        /*!
         * @brief Authenticate another signal. Which is to connect a function that basically emits the other signal.
         * @param signal The signal to connect to the signal.
         * @return The SlotId of the slot you just connected.
         */
        SlotId Connect(Signal<Args...> const& signal)
        {
            return Connect([=](Args... args) { signal.Emit(args...); });
        }

        SlotId Connect(Slot const& slot)
        {
            mSlots.insert(std::make_pair(++mSlotId, slot));
            return mSlotId;
        }

        void Disconnect(SlotId slotId)
        {
            mSlots.erase(slotId);
        }

        void Emit(Args... args)
        {
            for (auto &slot : mSlots)
            {
                slot.second(args...);
            }
        }

    private:
        std::unordered_map<SlotId, Slot> mSlots;
        SlotId mSlotId{0};
    };
}
#endif //WSCONNECT_SIGNALSSLOTS_H
