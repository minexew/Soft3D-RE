#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace soft3d {

template <typename T>
class StateSet;

template <typename T>
class State {
public:
    virtual ~State() = default;

    virtual void Enter(T* owner) = 0;
    virtual void Exit(T* owner) = 0;
    virtual State<T>* Update(T* owner) = 0;

    virtual void Register(StateSet<T>* states, int index)
    {
        states->RegisterState(index, this);
        enabled = true;
    }

    State<T>* GetNext() const { return next; }
    void SetNext(State<T>* value) { next = value; }
    bool IsEnabled() const { return enabled; }

private:
    bool enabled = false;
    State<T>* next = nullptr;
};

template <typename T>
class StateSet {
public:
    virtual ~StateSet() = default;

    virtual State<T>* GetInitialState(T*)
    {
        return GetRegisteredState(0);
    }

    virtual void InitializeStates(T*) {}

    virtual void RegisterState(int index, State<T>* state)
    {
        if (index < 0) {
            return;
        }
        const std::size_t position = static_cast<std::size_t>(index);
        if (states.size() <= position) {
            states.resize(position + 1, nullptr);
        }
        states[position] = state;
    }

    State<T>* GetRegisteredState(int index) const
    {
        if (index < 0 || static_cast<std::size_t>(index) >= states.size()) {
            return nullptr;
        }
        return states[static_cast<std::size_t>(index)];
    }

private:
    std::vector<State<T>*> states;
};

template <typename T>
class StateMachine {
public:
    explicit StateMachine(StateSet<T>* states) : stateSet(states) {}
    virtual ~StateMachine() = default;

    void Reset(T* owner)
    {
        if (current != nullptr) {
            ExitChain(current, owner);
        }
        current = stateSet != nullptr ? stateSet->GetInitialState(owner) : nullptr;
        if (current != nullptr) {
            EnterChain(current, owner);
        }
    }

    void Exit(T* owner)
    {
        if (current != nullptr) {
            ExitChain(current, owner);
        }
    }

    void Update(T* owner)
    {
        State<T>* next = current != nullptr ? FindTransition(current, owner) : nullptr;
        if (next == nullptr) {
            return;
        }
        ExitChain(current, owner);
        EnterChain(next, owner);
        pendingState.reset();
    }

    void SetPendingState(std::unique_ptr<State<T>> state)
    {
        pendingState = std::move(state);
    }

    State<T>* GetCurrentState() const { return current; }
    State<T>* GetPendingState() const { return pendingState.get(); }
    StateSet<T>* GetStateSet() const { return stateSet; }

private:
    void EnterChain(State<T>* state, T* owner)
    {
        if (state->GetNext() != nullptr) {
            EnterChain(state->GetNext(), owner);
        }
        state->Enter(owner);
        current = state;
    }

    void ExitChain(State<T>* state, T* owner)
    {
        state->Exit(owner);
        if (state->GetNext() != nullptr) {
            ExitChain(state->GetNext(), owner);
        }
    }

    State<T>* FindTransition(State<T>* state, T* owner)
    {
        State<T>* next = nullptr;
        if (state->GetNext() != nullptr) {
            next = FindTransition(state->GetNext(), owner);
        }
        if (next == nullptr) {
            next = state->Update(owner);
        }
        return next;
    }

    State<T>* current = nullptr;
    std::unique_ptr<State<T>> previousState;
    std::unique_ptr<State<T>> pendingState;
    StateSet<T>* stateSet;
};

} // namespace soft3d
