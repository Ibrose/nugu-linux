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

#ifndef __NUGU_CAPABILITY_H__
#define __NUGU_CAPABILITY_H__

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <base/nugu_event.h>
#include <base/nugu_network_manager.h>
#include <clientkit/capability_interface.hh>

namespace NuguClientKit {

/**
 * @file capability.hh
 * @defgroup Capability Capability
 * @ingroup SDKNuguClientKit
 * @brief base capability class
 *
 * A base class of all capability agents which inherit ICapabilityInterface
 * and implements required functions basically.
 *
 * @{
 */

/**
 * @brief Capability
 */
class Capability;

/**
 * @brief CapabilityEvent
 */
class CapabilityEvent {
public:
    CapabilityEvent(const std::string& name, Capability* cap);
    virtual ~CapabilityEvent();

    /**
     * @brief Get event name
     * @return event name
     */
    std::string getName();

    /**
     * @brief Get dialog request id
     * @return dialog request id
     */
    std::string getDialogRequestId();

    /**
     * @brief Get message id
     * @return message id
     */
    std::string getMessageId();

    /**
     * @brief Set dialog request id.
     * @param[in] id dialog request id
     */
    void setDialogRequestId(const std::string& id);

    /**
     * @brief Set event type.
     * @param[in] type event type
     */
    void setType(enum nugu_event_type type);

    /**
     * @brief Close event forcibly.
     */
    void forceClose();

    /**
     * @brief Send event to server.
     * @param[in] context context info
     * @param[in] payload payload info
     */
    void sendEvent(const std::string& context, const std::string& payload);

    /**
     * @brief Send attachment event to server.
     * @param[in] is_end whether final attachment event
     * @param[in] size attachment data size
     * @param[in] data attachment data
     */
    void sendAttachmentEvent(bool is_end, size_t size, unsigned char* data);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

/**
 * @brief Capability
 */
class Capability : virtual public ICapabilityInterface {
public:
    Capability(const std::string& name, const std::string& ver = "1.0");
    virtual ~Capability();

    /**
     * @brief Set INuguCoreContainer for using functions in NuguCore.
     * @param[in] core_container NuguCoreContainer instance
     */
    void setNuguCoreContainer(INuguCoreContainer* core_container) override;

    /**
     * @brief Initialize the current object.
     */
    void initialize() override;

    /**
     * @brief Deinitialize the current object.
     */
    void deInitialize() override;

    /**
     * @brief Set capability suspend policy
     * @param[in] policy suspend policy
     */
    void setSuspendPolicy(SuspendPolicy policy = SuspendPolicy::STOP) override;

    /**
     * @brief Suspend current action
     */
    void suspend() override;

    /**
     * @brief Restore previous suspended action
     */
    void restore() override;

    /**
     * @brief Add event result callback for error handling
     * @param[in] ename event name
     * @param[in] callback event result callback
     */
    void addEventResultCallback(const std::string& ename, EventResultCallback callback) override;

    /**
     * @brief Remove event result callback
     * @param[in] ename event name
     */
    void removeEventResultCallback(const std::string& ename) override;

    /**
     * @brief Notify event result
     * @param[in] event_desc event result description (format: 'cname.ename.msgid.dialogid.success.code')
     */
    void notifyEventResult(const std::string& event_desc) override;

    /**
     * @brief Notify event response info.
     * @param[in] msg_id message id which is sent with event
     * @param[in] data raw data which is received from server about event (json format)
     * @param[in] success whether receive event response
     */
    void notifyEventResponse(const std::string& msg_id, const std::string& data, bool success) override;

    /**
     * @brief Add event name and directive name for referred dialog request id.
     * @param[in] ename event name
     * @param[in] dname directive name
     */
    void addReferrerEvents(const std::string& ename, const std::string& dname);

    /**
     * @brief Add blocking policy for directive name
     * @param[in] dname directive name
     * @param[in] policy BlockingPolicy information
     */
    void addBlockingPolicy(const std::string& dname, BlockingPolicy policy);

    /**
     * @brief Get referred dialog request id.
     * @param[in] ename event name
     * @return referred dialog request id
     */
    std::string getReferrerDialogRequestId(const std::string& ename);

    /**
     * @brief Set referred dialog request id.
     * @param[in] dname directive name
     * @param[in] id referred dialog request id
     */
    void setReferrerDialogRequestId(const std::string& dname, const std::string& id);

    /**
     * @brief Set the capability name of the current object.
     * @param[in] name capability name
     */
    void setName(const std::string& name);

    /**
     * @brief Get the capability name of the current object.
     * @return capability name of the object
     */
    std::string getName() override;

    /**
     * @brief Set the capability version of the current object.
     * @param[in] ver capability version
     */
    void setVersion(const std::string& ver);

    /**
     * @brief Get the capability version of the current object.
     * @return capability version of the object
     */
    std::string getVersion() override;

    /**
     * @brief Get play service id which is managed by play stack control.
     * @param[in] playstack_control json object which has playStackControl field
     * @return current play service id
     */
    std::string getPlayServiceIdInStackControl(const Json::Value& playstack_control);

    /**
     * @brief Get play service id which is managed by play stack control.
     * * @param[in] payload raw json data
     * @return current play service id
     */
    std::string getPlayServiceIdInStackControl(const char* payload);

    /**
     * @brief Get interaction mode which is included in interactionControl.
     * @return interaction mode (NONE, MULTI_TURN,...)
     */
    InteractionMode getInteractionMode(const Json::Value& interaction_control);

    /**
     * @brief Receive a directive preprocessing request from Directive sequencer.
     * @param[in] ndir directive
     */
    void preprocessDirective(NuguDirective* ndir) override;

    /**
     * @brief Receive a directive cancellation from the Directive sequencer.
     * @param[in] ndir directive
     */
    void cancelDirective(NuguDirective* ndir) override;

    /**
     * @brief Receive a directive processing request from Directive sequencer.
     * @param[in] ndir directive
     */
    void processDirective(NuguDirective* ndir) override final;

    /**
     * @brief Destroy directive received from Directive Sequencer.
     * @param[in] ndir directive
     * @param[in] is_cancel whether to destroy by cancel
     */
    void destroyDirective(NuguDirective* ndir, bool is_cancel = false);

    /**
     * @brief Get directive received from Directive Sequencer.
     * @return received directive
     */
    NuguDirective* getNuguDirective();

    /**
     * @brief Send event to server.
     * @param[in] name event name
     * @param[in] context context info
     * @param[in] payload payload info
     * @return event's request dialog id
     */
    std::string sendEvent(const std::string& name, const std::string& context, const std::string& payload, EventResultCallback cb = nullptr);

    /**
     * @brief Send event to server.
     * @param[in] event CapabilityEvent instance
     * @param[in] context context info
     * @param[in] payload payload info
     */
    void sendEvent(CapabilityEvent* event, const std::string& context, const std::string& payload, EventResultCallback cb = nullptr);

    /**
     * @brief Send attachment event to server.
     * @param[in] event CapabilityEvent instance
     * @param[in] is_end whether final attachment event
     * @param[in] size attachment data size
     * @param[in] data attachment data
     */
    void sendAttachmentEvent(CapabilityEvent* event, bool is_end, size_t size, unsigned char* data);

    /**
     * @brief It is possible to share own property value among objects.
     * @param[in] property capability property
     * @param[in] values capability property value
     * @return property get result
     * @retval true The property is valid
     * @retval false The property is invalid
     */
    bool getProperty(const std::string& property, std::string& value) override;

    /**
     * @brief It is possible to share own property values among objects.
     * @param[in] property capability property
     * @param[in] values capability property values
     * @return property get result
     * @retval true The property is valid
     * @retval false The property is invalid
     */
    bool getProperties(const std::string& property, std::list<std::string>& values) override;

    /**
     * @brief Set the listener object.
     * @param[in] clistener listener
     */
    void setCapabilityListener(ICapabilityListener* clistener) override;

    /**
     * @brief Set directive cancel policy
     * @param[in] cancel_previous_dialog whether canceling previous dialog or not
     * @param[in] cancel_policy policy object
     */
    void setCancelPolicy(bool cancel_previous_dialog, DirectiveCancelPolicy&& cancel_policy = { true }) override;

    /**
     * @brief Process command from other objects.
     * @param[in] from capability who send the command
     * @param[in] command command
     * @param[in] param command parameter
     * @return command result
     * @retval true The command is valid
     * @retval false The command is invalid
     */
    bool receiveCommand(const std::string& from, const std::string& command, const std::string& param) override;

    /**
     * @brief Process command received from capability manager.
     * @param[in] command command
     * @param[in] param command parameter
     */
    void receiveCommandAll(const std::string& command, const std::string& param) override;

    /**
     * @brief Parsing directive and do the required action.
     * @param[in] dname directive name
     * @param[in] message directive data
     */
    virtual void parsingDirective(const char* dname, const char* message);

    /**
     * @brief Get current context info.
     * @return context info
     */
    virtual std::string getContextInfo();

    /**
     * @brief Update the compact context information of the capability agent.
     * @param[in] ctx capability agent's context
     */
    virtual void updateCompactContext(Json::Value& ctx);

    /**
     * @brief Get ICapabilityHelper instance for using NuguCore functions.
     * @return ICapabilityHelper instance
     */
    ICapabilityHelper* getCapabilityHelper();

protected:
    /** @brief whether capability initialized */
    bool initialized = false;

    /** @brief whether destroy received directive by agent */
    bool destroy_directive_by_agent = false;

    /** @brief whether capability suspend */
    bool suspended = false;

    /** @brief INuguCoreContainer instance for using NuguCore functions */
    INuguCoreContainer* core_container = nullptr;

    /** @brief ICapabilityHelper instance for using NuguCore functions */
    ICapabilityHelper* capa_helper = nullptr;

    /** @brief IPlaySyncManager instance for using playsync management */
    IPlaySyncManager* playsync_manager = nullptr;

    /** @brief IFocusManager instance for using audio focus */
    IFocusManager* focus_manager = nullptr;

    /** @brief ISessionManager instance for using session management */
    ISessionManager* session_manager = nullptr;

    /** @brief IInteractionControlManager instance for using interaction control management */
    IInteractionControlManager* interaction_control_manager = nullptr;

    /** @brief IDirectiveSequencer instance for directive sequence management */
    IDirectiveSequencer* directive_sequencer = nullptr;

    /** @brief IRoutineManager instance for using routine management */
    IRoutineManager* routine_manager = nullptr;

    /** @brief SuspendPolicy variable for deciding suspend action (default:STOP) */
    SuspendPolicy suspend_policy = SuspendPolicy::STOP;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
    std::map<std::string, EventResultCallback> event_result_cbs;
};

/**
 * @}
 */

} // NuguClientKit

#endif
