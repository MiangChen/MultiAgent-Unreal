(function () {
    const UI = window.MultiAgentUI;
    const data = window.MA_CONSOLE_DATA || {};
    const port = data.port || '';
    const skillLists = data.skillLists || {};
    const taskGraphs = data.taskGraphs || {};
    const skillAllocations = data.skillAllocations || {};

    let messagesDiv;
    let emptyState;
    let statusDot;
    let statusText;
    let emptyStateTitle;
    let emptyStateSubtitle;
    let selectionStatus;
    let selectionKind;
    let selectionTitle;
    let selectionDesc;
    let selectionMeta;
    let selectionHint;
    let selectionDiagramWrap;
    let logConnection;
    let logConnectionText;
    let isUEConnected = false;
    let selectedConsoleItem = null;

    function buildTaskGraphMermaid(taskEntry) {
        const graph = taskEntry?.data?.task_graph;
        if (!graph?.nodes?.length) return null;
        let code = 'graph TD\n';
        graph.nodes.forEach((node) => {
            const skills = (node.required_skills || []).map((skill) => skill.skill_name).join(', ');
            const desc = (node.description || '').replace(/"/g, '\\"');
            const details = [desc, skills].filter(Boolean).join('<br/>');
            code += `    ${node.task_id}["${node.task_id}: ${details}"]\n`;
        });
        (graph.edges || []).forEach((edge) => {
            const arrow = edge.type === 'parallel' ? ' -.-> ' : ' --> ';
            const label = edge.condition ? `${edge.type}: ${edge.condition}` : edge.type;
            code += `    ${edge.from}${arrow}|${label}| ${edge.to}\n`;
        });
        return code;
    }

    function buildDemoFlowMermaid() {
        return [
            'flowchart LR',
            '    A["Review Plan<br/>Task Graph"] --> B["Review Allocation<br/>Skill Allocation"]',
            '    B --> C["Direct Execute<br/>Skill List"]',
            '    D["End-to-End Demo<br/>Guided page"] -. Step-by-step flow .-> A',
            '    D -. Chains review and execution .-> B',
            '    D -. Ends in execution .-> C'
        ].join('\n');
    }

    function buildTaskGraphMeta(taskEntry) {
        const graph = taskEntry?.data?.task_graph || {};
        return [
            `${graph.nodes?.length || 0} tasks`,
            `${graph.edges?.length || 0} dependencies`,
            'HITL review'
        ];
    }

    function buildSkillAllocationMeta(entry) {
        return UI.buildTimelineMeta(entry?.data || {}, 'HITL review');
    }

    function buildSkillListMeta(entry) {
        return UI.buildTimelineMeta(entry?.data || {}, 'direct execute');
    }

    function getSelectionPayload(type, key) {
        if (type === 'task_graph') {
            const entry = taskGraphs[key];
            if (!entry) return null;
            return {
                kind: 'Review Plan',
                kindClass: 'plan',
                status: 'Showing the task dependency DAG',
                title: entry.name,
                desc: entry.description || 'No description',
                meta: buildTaskGraphMeta(entry),
                hint: 'After sending, this enters the HITL review queue. Continue only after reviewing and confirming the task graph in UE.',
                diagramKind: 'mermaid',
                mermaidCode: buildTaskGraphMermaid(entry)
            };
        }
        if (type === 'skill_allocation') {
            const entry = skillAllocations[key];
            if (!entry) return null;
            return {
                kind: 'Review Allocation',
                kindClass: 'allocation',
                status: 'Showing the robot allocation timeline',
                title: entry.name,
                desc: entry.description || 'No description',
                meta: buildSkillAllocationMeta(entry),
                hint: 'After sending, this enters the HITL review queue. Continue only after reviewing and confirming the gantt allocation in UE.',
                diagramKind: 'timeline',
                timelineTitle: entry.name,
                timelineSubtitle: entry.description || 'Skill allocation timeline',
                timelineData: entry.data || {}
            };
        }
        if (type === 'skill_list') {
            const entry = skillLists[key];
            if (!entry) return null;
            return {
                kind: 'Direct Execute',
                kindClass: 'execute',
                status: 'Showing the timeline for direct dispatch',
                title: entry.name,
                desc: entry.description || 'No description',
                meta: buildSkillListMeta(entry),
                hint: 'This path bypasses HITL review and is dispatched directly to the Platform execution pipeline.',
                diagramKind: 'timeline',
                timelineTitle: entry.name,
                timelineSubtitle: entry.description || 'Direct execution timeline',
                timelineData: entry.data || {}
            };
        }
        if (type === 'demo_flow') {
            return {
                kind: 'End-to-End Demo',
                kindClass: 'demo',
                status: 'Showing the guided demo workflow',
                title: 'Guided Demo Page',
                desc: 'This entry does not dispatch tasks directly. It switches to the Demo page and walks through review plan, review allocation, and final execution.',
                meta: [
                    '4 semantic groups',
                    'step-driven',
                    'teaching / demo'
                ],
                hint: 'Use the left-side button to jump to /demo and walk through task-graph review, allocation review, and execution step by step.',
                diagramKind: 'mermaid',
                mermaidCode: buildDemoFlowMermaid()
            };
        }
        return null;
    }

    function updateActiveSelectionCard() {
        document.querySelectorAll('.preview-item').forEach((item) => {
            const isActive = selectedConsoleItem
                && item.dataset.previewType === selectedConsoleItem.type
                && item.dataset.previewKey === selectedConsoleItem.key;
            item.classList.toggle('active', !!isActive);
        });
    }

    function setSelectionKind(kind, kindClass) {
        selectionKind.className = `preview-panel-kind ${kindClass || ''}`.trim();
        selectionKind.textContent = kind;
    }

    function renderSelectedConsoleItem() {
        const payload = selectedConsoleItem
            ? getSelectionPayload(selectedConsoleItem.type, selectedConsoleItem.key)
            : null;

        updateActiveSelectionCard();

        if (!payload) {
            setSelectionKind('Not Selected', '');
            selectionStatus.textContent = 'Select any item from the four groups on the left to preview it here';
            selectionTitle.textContent = 'Choose an item on the left';
            selectionDesc.textContent = 'The left panel organizes the four entry groups, the middle panel previews them, and the right panel keeps compact send status logs.';
            selectionMeta.innerHTML = '';
            selectionHint.textContent = 'Click a card to preview it. Click a button only when you want to send or navigate.';
            selectionDiagramWrap.innerHTML = `
                <div class="preview-empty">
                    <strong>No item selected yet</strong>
                    <span>Select a review plan, review allocation, direct execute example, or demo entry on the left to render its visualization here.</span>
                </div>
            `;
            return;
        }

        setSelectionKind(payload.kind, payload.kindClass);
        selectionStatus.textContent = payload.status;
        selectionTitle.textContent = payload.title;
        selectionDesc.textContent = payload.desc;
        selectionMeta.innerHTML = payload.meta.map((item) => `<span class="preview-chip">${UI.escapeHtml(item)}</span>`).join('');
        selectionHint.textContent = payload.hint;

        if (payload.diagramKind === 'timeline') {
            UI.renderTimelineGantt(selectionDiagramWrap, {
                data: payload.timelineData,
                title: payload.timelineTitle,
                subtitle: payload.timelineSubtitle,
                badges: payload.meta,
                sidebarWidth: 220,
                columnWidth: 150
            });
            return;
        }

        if (!payload.mermaidCode) {
            selectionDiagramWrap.innerHTML = '<div class="preview-empty"><strong>No visualization available</strong><span>The current entry has no Mermaid diagram to render.</span></div>';
            return;
        }
        const rendered = UI.renderMermaidInto(selectionDiagramWrap, payload.mermaidCode);
        if (!rendered) {
            selectionDiagramWrap.innerHTML = '<div class="preview-empty"><strong>Visualization engine loading</strong><span>The current selection will render automatically after Mermaid finishes initializing.</span></div>';
        }
    }

    function selectConsoleItem(type, key) {
        selectedConsoleItem = { type, key };
        renderSelectedConsoleItem();
    }

    function isFailureContent(content) {
        if (content == null) return false;
        if (typeof content === 'string') {
            return /(失败|error|rejected|reject|拒绝|断开|断连|timeout|超时)/i.test(content);
        }
        if (typeof content !== 'object') return false;
        if (content.error) return true;
        if (Array.isArray(content.errors) && content.errors.length) return true;
        if (typeof content.approved === 'boolean' && content.approved === false) return true;
        if (content.status && /fail|failed|error|rejected|timeout/i.test(String(content.status))) return true;
        if (content.result && /fail|failed|error/i.test(String(content.result))) return true;
        if (content.review_result && /reject|rejected|fail|failed|error/i.test(String(content.review_result))) return true;
        if (content.message && /(失败|error|rejected|reject|拒绝|断开|断连|timeout|超时)/i.test(String(content.message))) return true;
        if (content.note && /(失败|error|rejected|reject|拒绝)/i.test(String(content.note))) return true;
        return false;
    }

    function buildTopicSearchText(topic, title, content) {
        const parts = [topic, title];
        if (typeof content === 'string') {
            parts.push(content);
        } else if (content && typeof content === 'object') {
            try {
                parts.push(JSON.stringify(content));
            } catch (_err) {
                // Ignore stringify issues and keep lightweight fallbacks.
            }
        }
        return parts.filter(Boolean).join(' ').toLowerCase();
    }

    function classifyBusinessTopic(topic, title, content) {
        const searchText = buildTopicSearchText(topic, title, content);
        if (searchText.includes('task_graph') || String(topic).includes('review plan')) {
            return { label: 'Review Plan', className: 'plan' };
        }
        if (searchText.includes('skill_allocation') || searchText.includes('auto skill allocation') || String(topic).includes('review allocation')) {
            return { label: 'Review Allocation', className: 'allocation' };
        }
        if (searchText.includes('skill_list') || searchText.includes('auto execute skill list') || String(topic).includes('direct execute')) {
            return { label: 'Direct Execute', className: 'execute' };
        }
        if (searchText.includes('demo') || String(topic).includes('end-to-end demo')) {
            return { label: 'End-to-End Demo', className: 'demo' };
        }
        if (searchText.includes('user_instruction') || String(topic).includes('utilities')) {
            return { label: 'Utilities', className: 'tool' };
        }
        if (searchText.includes('scene_change')) {
            return { label: 'Scene Change', className: 'system' };
        }
        if (searchText.includes('connection')) {
            return { label: 'Connection', className: 'system' };
        }
        if (searchText.includes('hitl') || searchText.includes('review_response') || searchText.includes('decision_response')) {
            return { label: 'Review Response', className: 'system' };
        }
        return { label: 'System', className: 'system' };
    }

    function addMessage(topic, title, content, direction, level = 'normal') {
        if (emptyState) emptyState.style.display = 'none';
        const dirClass = level === 'error' || isFailureContent(content)
            ? 'error'
            : (direction === 'system' ? 'system' : (direction === 'outgoing' ? 'outgoing' : 'incoming'));
        const dirLabel = dirClass === 'error'
            ? 'Error'
            : (direction === 'system' ? 'State' : (direction === 'outgoing' ? 'Sent' : 'Received'));
        const topicMeta = classifyBusinessTopic(topic, title, content);
        const msg = document.createElement('div');
        msg.className = `message ${dirClass} topic-${topicMeta.className}`;
        msg.innerHTML = `
            <div class="message-header">
                <div class="message-header-left">
                    <span class="message-type">${dirLabel}</span>
                    <span class="message-topic-badge ${topicMeta.className}">${UI.escapeHtml(topicMeta.label)}</span>
                </div>
                <span class="message-time">${new Date().toLocaleTimeString()}</span>
            </div>
            <div class="message-title">${UI.escapeHtml(title)}</div>
            <div class="message-content">${UI.escapeHtml(UI.summarizeContent(content))}</div>
        `;
        messagesDiv.insertBefore(msg, messagesDiv.firstChild);
        while (messagesDiv.children.length > 100) {
            messagesDiv.removeChild(messagesDiv.lastChild);
        }
    }

    function applyUEConnectionStatus(connected) {
        const previous = isUEConnected;
        isUEConnected = connected;
        if (connected) {
            statusDot.className = 'dot connected';
            statusText.textContent = 'UE5 connected';
            logConnection.classList.add('connected');
            logConnectionText.textContent = 'UE5 connected';
            if (emptyStateTitle) emptyStateTitle.textContent = '✅ UE5 connected';
            if (emptyStateSubtitle) emptyStateSubtitle.textContent = 'Waiting for you to send tasks. Messages will appear here in real time.';
        } else {
            statusDot.className = 'dot disconnected';
            statusText.textContent = `Server Running on :${port} | Waiting for UE5 connection...`;
            logConnection.classList.remove('connected');
            logConnectionText.textContent = 'Waiting for UE5 connection';
            if (emptyStateTitle) emptyStateTitle.textContent = 'Waiting for UE5 simulator connection...';
            if (emptyStateSubtitle) emptyStateSubtitle.textContent = 'Messages will appear here in real time.';
        }
        if (connected && !previous) addMessage('Connection', 'UE5 simulator', 'Connected. Tasks can now be sent and responses can be received.', 'system');
        else if (!connected && previous) addMessage('Connection', 'UE5 simulator', 'Connection lost. Waiting for UE5 to poll again.', 'system', 'error');
    }

    async function fetchUEConnectionStatus() {
        try {
            const response = await fetch('/api/ue_status');
            if (!response.ok) return;
            const payload = await response.json();
            applyUEConnectionStatus(!!payload.connected);
        } catch (_err) {
            // Ignore transient fetch errors.
        }
    }

    async function sendTaskGraph(taskKey) {
        try {
            const response = await fetch('/api/send_task_graph', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ task_key: taskKey })
            });
            addMessage('Review Plan', taskGraphs[taskKey]?.name || taskKey, await response.json(), 'outgoing');
        } catch (error) {
            addMessage('Review Plan', taskGraphs[taskKey]?.name || taskKey, `Send failed: ${error.message}`, 'system', 'error');
        }
    }

    async function sendSkillAllocation(allocationKey) {
        try {
            const response = await fetch('/api/send_skill_allocation', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ allocation_key: allocationKey })
            });
            addMessage('Review Allocation', skillAllocations[allocationKey]?.name || allocationKey, await response.json(), 'outgoing');
        } catch (error) {
            addMessage('Review Allocation', skillAllocations[allocationKey]?.name || allocationKey, `Send failed: ${error.message}`, 'system', 'error');
        }
    }

    async function sendSkill(skillKey) {
        try {
            const response = await fetch('/api/send_skill', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ skill_key: skillKey })
            });
            addMessage('Direct Execute', skillLists[skillKey]?.name || skillKey, await response.json(), 'outgoing');
        } catch (error) {
            addMessage('Direct Execute', skillLists[skillKey]?.name || skillKey, `Send failed: ${error.message}`, 'system', 'error');
        }
    }

    async function sendRequestUserCommand() {
        try {
            const response = await fetch('/api/send_request_user_command', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({})
            });
            addMessage('Utilities', 'Request User Command', await response.json(), 'outgoing');
        } catch (error) {
            addMessage('Utilities', 'Request User Command', `Send failed: ${error.message}`, 'system', 'error');
        }
    }

    function openDemoPage() {
        addMessage('End-to-End Demo', 'Open guided demo page', 'Switching to /demo to guide review plan, review allocation, and final execution.', 'system');
        window.location.href = '/demo';
    }

    function clearMessages() {
        messagesDiv.innerHTML = '';
        emptyState.style.display = 'block';
        messagesDiv.appendChild(emptyState);
        fetchUEConnectionStatus();
    }

    function buildPreviewItem({ type, key, title, description, meta, hint, accentClass, actionLabel, onAction }) {
        const item = document.createElement('div');
        item.className = `preview-item ${accentClass}`;
        item.dataset.previewType = type;
        item.dataset.previewKey = key;
        item.innerHTML = `
            <div class="preview-item-head">
                <div>
                    <div class="preview-item-title">${UI.escapeHtml(title)}</div>
                    <div class="preview-item-desc">${UI.escapeHtml(description)}</div>
                </div>
            </div>
            <div class="preview-item-meta">${meta.map((value) => `<span class="preview-chip">${UI.escapeHtml(value)}</span>`).join('')}</div>
            <div class="preview-item-hint">${UI.escapeHtml(hint)}</div>
            <div class="preview-item-actions">
                <button class="preview-action-btn ${accentClass}">${UI.escapeHtml(actionLabel)}</button>
            </div>
        `;
        item.onclick = () => selectConsoleItem(type, key);
        item.querySelector('.preview-action-btn').onclick = (event) => {
            event.stopPropagation();
            onAction();
        };
        return item;
    }

    function initConsoleLists() {
        const taskButtonsDiv = document.getElementById('task-buttons');
        const allocationButtonsDiv = document.getElementById('allocation-buttons');
        const skillButtonsDiv = document.getElementById('skill-buttons');
        const demoFlowButtonsDiv = document.getElementById('demo-flow-buttons');

        Object.entries(taskGraphs).forEach(([key, task]) => {
            taskButtonsDiv.appendChild(buildPreviewItem({
                type: 'task_graph',
                key,
                title: `📊 ${task.name}`,
                description: task.description,
                meta: buildTaskGraphMeta(task),
                hint: 'Click the card to preview the DAG. Click the button to send it into HITL plan review.',
                accentClass: 'plan',
                actionLabel: 'Send for Review',
                onAction: () => sendTaskGraph(key)
            }));
        });

        Object.entries(skillAllocations).forEach(([key, allocation]) => {
            allocationButtonsDiv.appendChild(buildPreviewItem({
                type: 'skill_allocation',
                key,
                title: `🎯 ${allocation.name}`,
                description: allocation.description,
                meta: buildSkillAllocationMeta(allocation),
                hint: 'Click the card to preview the gantt. Click the button to send it into HITL allocation review.',
                accentClass: 'allocation',
                actionLabel: 'Send for Review',
                onAction: () => sendSkillAllocation(key)
            }));
        });

        Object.entries(skillLists).forEach(([key, skill]) => {
            skillButtonsDiv.appendChild(buildPreviewItem({
                type: 'skill_list',
                key,
                title: `⚡ ${skill.name}`,
                description: skill.description,
                meta: buildSkillListMeta(skill),
                hint: 'Click the card to preview the execution timeline. Click the button to dispatch directly to the Platform.',
                accentClass: 'execute',
                actionLabel: 'Execute Now',
                onAction: () => sendSkill(key)
            }));
        });

        demoFlowButtonsDiv.appendChild(buildPreviewItem({
            type: 'demo_flow',
            key: 'guided_demo',
            title: '🎬 Guided Demo Page',
            description: 'Walk through plan review, allocation review, and final execution step by step instead of dispatching directly here.',
            meta: ['guided mode', 'step by step', 'end-to-end'],
            hint: 'Click the card to preview the full workflow first, then click the button to open /demo.',
            accentClass: 'demo',
            actionLabel: 'Open Demo',
            onAction: openDemoPage
        }));

        window.sendRequestUserCommand = sendRequestUserCommand;
        window.clearMessages = clearMessages;
    }

    function initEventStream() {
        const evtSource = new EventSource('/api/messages');
        evtSource.onopen = () => fetchUEConnectionStatus();
        evtSource.onmessage = (event) => {
            const payload = JSON.parse(event.data);
            addMessage(payload.type || 'UE Response', payload.title || 'Untitled Message', payload.content, payload.direction || 'incoming');
        };
        evtSource.onerror = () => {
            applyUEConnectionStatus(false);
            statusText.textContent = `Server Running on :${port} | Connection lost, waiting to reconnect...`;
        };
    }

    function cacheDom() {
        messagesDiv = document.getElementById('messages');
        emptyState = document.getElementById('empty-state');
        statusDot = document.getElementById('status-dot');
        statusText = document.getElementById('status-text');
        emptyStateTitle = document.getElementById('empty-state-title');
        emptyStateSubtitle = document.getElementById('empty-state-subtitle');
        selectionStatus = document.getElementById('selection-status');
        selectionKind = document.getElementById('selection-kind');
        selectionTitle = document.getElementById('selection-title');
        selectionDesc = document.getElementById('selection-desc');
        selectionMeta = document.getElementById('selection-meta');
        selectionHint = document.getElementById('selection-hint');
        selectionDiagramWrap = document.getElementById('selection-diagram-wrap');
        logConnection = document.getElementById('log-connection');
        logConnectionText = document.getElementById('log-connection-text');
        statusText.textContent = `Server Running on :${port} | Waiting for UE5 connection...`;
    }

    function pickDefaultSelection() {
        const firstTaskKey = Object.keys(taskGraphs)[0];
        if (firstTaskKey) return { type: 'task_graph', key: firstTaskKey };

        const firstAllocationKey = Object.keys(skillAllocations)[0];
        if (firstAllocationKey) return { type: 'skill_allocation', key: firstAllocationKey };

        const firstSkillKey = Object.keys(skillLists)[0];
        if (firstSkillKey) return { type: 'skill_list', key: firstSkillKey };

        return { type: 'demo_flow', key: 'guided_demo' };
    }

    function init() {
        cacheDom();
        initConsoleLists();
        initEventStream();
        const defaultSelection = pickDefaultSelection();
        if (defaultSelection) {
            selectConsoleItem(defaultSelection.type, defaultSelection.key);
        } else {
            renderSelectedConsoleItem();
        }
        fetchUEConnectionStatus();
        setInterval(fetchUEConnectionStatus, 1000);
    }

    window.addEventListener('DOMContentLoaded', init);
})();
