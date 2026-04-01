/*************************************************************************
@file		EventSystem.h
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the function declarations for the event system,
            including subscribing to events, firing events, and clearing all events.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#pragma once
#include <functional>
#include <unordered_map>
#include <vector>

enum class GameEvent {
    ON_DASH,
    ON_ATTACK_HIT,
    ON_PARRY_SUCCESS
};

struct EventData {
    float playerX{}, playerY{};
    float dirX{}, dirY{};
    float damage{};
    void* targetEnemy{};
};

struct GameEventHash {
    size_t operator()(GameEvent e) const { return static_cast<size_t>(e); }
};

class EventSystem {
public:
    void Subscribe(GameEvent event, std::function<void(EventData const&)> callback) {
        m_listeners[event].push_back(callback);
    }

    void Fire(GameEvent event, EventData const& data) {
        auto it = m_listeners.find(event);
        if (it != m_listeners.end()) {
            for (auto& cb : it->second) {
                cb(data);
            }
        }
    }

    void ClearAll() {
        m_listeners.clear();
    }

private:
    std::unordered_map<GameEvent, std::vector<std::function<void(EventData const&)>>, GameEventHash> m_listeners;
};

extern EventSystem g_Events;
