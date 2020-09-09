/*
 * Copyright (c) 2019 SK Telecom Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>

#include "base/nugu_log.h"
#include "playsync_manager.hh"

namespace NuguCore {

PlaySyncManager::PlaySyncManager()
    : playstack_manager(std::unique_ptr<PlayStackManager>(new PlayStackManager()))
{
    playstack_manager->addListener(this);
}

PlaySyncManager::~PlaySyncManager()
{
    clearContainer();
    listener_map.clear();
}

void PlaySyncManager::setPlayStackManager(PlayStackManager* playstack_manager)
{
    if (!playstack_manager) {
        nugu_warn("The PlayStackManager instance is not exist.");
        return;
    }

    this->playstack_manager.reset(playstack_manager);
    this->playstack_manager->addListener(this);
}

void PlaySyncManager::addListener(const std::string& requester, IPlaySyncManagerListener* listener)
{
    if (requester.empty() || !listener) {
        nugu_warn("The requester or listener is not exist.");
        return;
    }

    listener_map.emplace(requester, listener);
}

void PlaySyncManager::removeListener(const std::string& requester)
{
    if (requester.empty()) {
        nugu_warn("The requester is not exist.");
        return;
    }

    listener_map.erase(requester);
}

int PlaySyncManager::getListenerCount()
{
    return listener_map.size();
}

void PlaySyncManager::prepareSync(const std::string& ps_id, NuguDirective* ndir)
{
    if (!playstack_manager->add(ps_id, ndir)) {
        nugu_warn("The condition is not satisfied to prepare sync.");
        return;
    }

    clearPostPonedRelease();

    if (!playstack_manager->isStackedCondition(ndir))
        clearContainer();

    std::string dir_groups = nugu_directive_peek_groups(ndir);
    PlaySyncContainer playsync_container;

    for (const auto& sync_capability : SYNC_CAPABILITAY_LIST)
        if (dir_groups.find(sync_capability) != std::string::npos)
            playsync_container.emplace(sync_capability, std::make_pair(PlaySyncState::Prepared, nullptr));

    playstack_map.emplace(ps_id, playsync_container);

    notifyStateChange(ps_id, PlaySyncState::Prepared);
}

void PlaySyncManager::startSync(const std::string& ps_id, const std::string& requester, void* extra_data)
{
    if (!isConditionToSyncAction(ps_id, requester, PlaySyncState::Synced))
        return;

    auto& playsync_container = playstack_map.at(ps_id);
    playsync_container[requester] = std::make_pair(PlaySyncState::Synced, extra_data);

    // It notify synced state only if all elements are synced.
    for (const auto& element : playsync_container)
        if (element.second.first != PlaySyncState::Synced)
            return;

    notifyStateChange(ps_id, PlaySyncState::Synced);
}

void PlaySyncManager::cancelSync(const std::string& ps_id, const std::string& requester)
{
    if (!isConditionToSyncAction(ps_id, requester, PlaySyncState::None))
        return;

    playstack_map.at(ps_id).erase(requester);
}

void PlaySyncManager::releaseSync(const std::string& ps_id, const std::string& requester)
{
    rawReleaseSync(ps_id, requester, PlayStackRemoveMode::Normal);
}

void PlaySyncManager::releaseSyncLater(const std::string& ps_id, const std::string& requester)
{
    rawReleaseSync(ps_id, requester, PlayStackRemoveMode::Later);
}

void PlaySyncManager::releaseSyncImmediately(const std::string& ps_id, const std::string& requester)
{
    rawReleaseSync(ps_id, requester, PlayStackRemoveMode::Immediately);
}

void PlaySyncManager::postPoneRelease()
{
    release_postponed = true;
}

void PlaySyncManager::continueRelease()
{
    if (release_postponed && postponed_release_func)
        postponed_release_func();

    clearPostPonedRelease();
}

void PlaySyncManager::stopHolding()
{
    playstack_manager->stopHolding();
}

void PlaySyncManager::resetHolding()
{
    playstack_manager->resetHolding();
}

bool PlaySyncManager::isConditionToHandlePrevDialog(NuguDirective* prev_ndir, NuguDirective* cur_ndir)
{
    return playstack_manager->isStackedCondition(cur_ndir) && !playstack_manager->hasExpectSpeech(prev_ndir);
}

bool PlaySyncManager::hasLayer(const std::string& ps_id, PlayStackLayer layer)
{
    return playstack_manager->getPlayStackLayer(ps_id) == layer;
}

std::vector<std::string> PlaySyncManager::getAllPlayStackItems()
{
    return playstack_manager->getAllPlayStackItems();
}

const PlaySyncManager::PlayStacks& PlaySyncManager::getPlayStacks()
{
    return playstack_map;
}

void PlaySyncManager::onStackAdded(const std::string& ps_id)
{
}

void PlaySyncManager::onStackRemoved(const std::string& ps_id)
{
    if (playstack_map.find(ps_id) == playstack_map.cend()) {
        nugu_warn("The PlaySyncContainer is not exist.");
        return;
    }

    auto& playsync_container = playstack_map.at(ps_id);

    for (auto& element : playsync_container)
        element.second.first = PlaySyncState::Released;

    notifyStateChange(ps_id, PlaySyncState::Released);

    playstack_map.erase(ps_id);
}

void PlaySyncManager::notifyStateChange(const std::string& ps_id, PlaySyncState state)
{
    if (playstack_map.find(ps_id) == playstack_map.cend()) {
        nugu_warn("The PlaySyncContainer is not exist.");
        return;
    }

    const auto& playsync_container = playstack_map.at(ps_id);

    for (const auto& element : playsync_container) {
        try {
            listener_map.at(element.first)->onSyncState(ps_id, state, element.second.second);
        } catch (std::out_of_range& exception) {
            nugu_warn("The requester is not exist.");
        }
    }
}

bool PlaySyncManager::isConditionToSyncAction(const std::string& ps_id, const std::string& requester, PlaySyncState state)
{
    if (ps_id.empty() || requester.empty() || playstack_map.find(ps_id) == playstack_map.cend()) {
        nugu_warn("The condition is not satisfied to sync action.");
        return false;
    }

    try {
        if (playstack_map.at(ps_id).at(requester).first == state) {
            nugu_warn("It's already done.");
            return false;
        }
    } catch (std::out_of_range& exception) {
        nugu_warn("The requester is not exist.");
        return false;
    }

    return true;
}

void PlaySyncManager::rawReleaseSync(const std::string& ps_id, const std::string& requester, PlayStackRemoveMode stack_remove_mode)
{
    if (!isConditionToSyncAction(ps_id, requester, PlaySyncState::Released))
        return;

    auto release_func = [=]() {
        playstack_manager->stopHolding();
        playstack_manager->remove(ps_id, stack_remove_mode);
    };

    if (release_postponed) {
        postponed_release_func = release_func;
        return;
    }

    release_func();
}

void PlaySyncManager::clearContainer()
{
    playstack_map.clear();
}

void PlaySyncManager::clearPostPonedRelease()
{
    postponed_release_func = nullptr;
    release_postponed = false;
}

} // NuguCore
