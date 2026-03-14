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
            '    A["待审核计划<br/>Task Graph"] --> B["待审核排班<br/>Skill Allocation"]',
            '    B --> C["直接执行示例<br/>Skill List"]',
            '    D["完整流程 Demo<br/>引导演示页"] -. 按步骤驱动 .-> A',
            '    D -. 串起审核与执行 .-> B',
            '    D -. 最终进入执行 .-> C'
        ].join('\n');
    }

    function buildTaskGraphMeta(taskEntry) {
        const graph = taskEntry?.data?.task_graph || {};
        return [
            `${graph.nodes?.length || 0} 个任务`,
            `${graph.edges?.length || 0} 条依赖`,
            'HITL 审核'
        ];
    }

    function buildSkillAllocationMeta(entry) {
        return UI.buildTimelineMeta(entry?.data || {}, 'HITL 审核');
    }

    function buildSkillListMeta(entry) {
        return UI.buildTimelineMeta(entry?.data || {}, '直接执行');
    }

    function getSelectionPayload(type, key) {
        if (type === 'task_graph') {
            const entry = taskGraphs[key];
            if (!entry) return null;
            return {
                kind: '待审核计划',
                kindClass: 'plan',
                status: '当前展示任务依赖 DAG',
                title: entry.name,
                desc: entry.description || '无描述',
                meta: buildTaskGraphMeta(entry),
                hint: '发送后进入 HITL 审核队列，需要在 UE 中查看任务图并确认后才继续。',
                diagramKind: 'mermaid',
                mermaidCode: buildTaskGraphMermaid(entry)
            };
        }
        if (type === 'skill_allocation') {
            const entry = skillAllocations[key];
            if (!entry) return null;
            return {
                kind: '待审核排班',
                kindClass: 'allocation',
                status: '当前展示机器人时间分配甘特图',
                title: entry.name,
                desc: entry.description || '无描述',
                meta: buildSkillAllocationMeta(entry),
                hint: '发送后进入 HITL 审核队列，需要在 UE 中查看甘特图并确认排班。',
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
                kind: '直接执行示例',
                kindClass: 'execute',
                status: '当前展示可直接下发的技能时间线',
                title: entry.name,
                desc: entry.description || '无描述',
                meta: buildSkillListMeta(entry),
                hint: '这一类不会进入 HITL 审核，发送后会直接下发到 Platform 执行链路。',
                diagramKind: 'timeline',
                timelineTitle: entry.name,
                timelineSubtitle: entry.description || 'Direct execution timeline',
                timelineData: entry.data || {}
            };
        }
        if (type === 'demo_flow') {
            return {
                kind: '完整流程 Demo',
                kindClass: 'demo',
                status: '当前展示引导演示的完整工作流',
                title: '引导演示页',
                desc: '这个入口不直接下发任务，而是切换到 Demo 页，按步骤串起待审核计划、待审核排班和最终执行。',
                meta: [
                    '4 个语义分区',
                    '按步骤驱动',
                    '教学 / 演示'
                ],
                hint: '点击左侧按钮会跳转到 /demo，引导你逐步发送任务图、排班和执行指令。',
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
            setSelectionKind('未选择', '');
            selectionStatus.textContent = '点击左侧 4 类入口中的任意条目，直接在这里查看';
            selectionTitle.textContent = '选择左侧条目';
            selectionDesc.textContent = '左侧负责组织 4 类入口，中间负责可视化预览，右侧负责精简发送状态。';
            selectionMeta.innerHTML = '';
            selectionHint.textContent = '点卡片查看结构，点按钮才触发发送或跳转。';
            selectionDiagramWrap.innerHTML = `
                <div class="preview-empty">
                    <strong>还没有选中任何条目</strong>
                    <span>左侧点击审核计划、审核排班、直接执行示例或 Demo 入口，中间会直接渲染可视化。</span>
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
            selectionDiagramWrap.innerHTML = '<div class="preview-empty"><strong>暂无可视化内容</strong><span>当前条目没有可用的 Mermaid 图。</span></div>';
            return;
        }
        const rendered = UI.renderMermaidInto(selectionDiagramWrap, payload.mermaidCode);
        if (!rendered) {
            selectionDiagramWrap.innerHTML = '<div class="preview-empty"><strong>可视化引擎加载中</strong><span>Mermaid 初始化完成后会自动渲染当前选择。</span></div>';
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
        if (searchText.includes('task_graph') || String(topic).includes('待审核计划')) {
            return { label: '待审核计划', className: 'plan' };
        }
        if (searchText.includes('skill_allocation') || searchText.includes('auto skill allocation') || String(topic).includes('待审核排班')) {
            return { label: '待审核排班', className: 'allocation' };
        }
        if (searchText.includes('skill_list') || searchText.includes('auto execute skill list') || String(topic).includes('直接执行示例')) {
            return { label: '直接执行示例', className: 'execute' };
        }
        if (searchText.includes('demo') || String(topic).includes('完整流程 Demo')) {
            return { label: '完整流程 Demo', className: 'demo' };
        }
        if (searchText.includes('user_instruction') || String(topic).includes('辅助工具')) {
            return { label: '辅助工具', className: 'tool' };
        }
        if (searchText.includes('scene_change')) {
            return { label: '场景变化', className: 'system' };
        }
        if (searchText.includes('连接') || searchText.includes('connection')) {
            return { label: '连接状态', className: 'system' };
        }
        if (searchText.includes('hitl') || searchText.includes('review_response') || searchText.includes('decision_response')) {
            return { label: '审核回传', className: 'system' };
        }
        return { label: '系统消息', className: 'system' };
    }

    function addMessage(topic, title, content, direction, level = 'normal') {
        if (emptyState) emptyState.style.display = 'none';
        const dirClass = level === 'error' || isFailureContent(content)
            ? 'error'
            : (direction === 'system' ? 'system' : (direction === 'outgoing' ? 'outgoing' : 'incoming'));
        const dirLabel = dirClass === 'error'
            ? '失败'
            : (direction === 'system' ? '状态' : (direction === 'outgoing' ? '发送' : '回传'));
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
            statusText.textContent = 'UE5 链接成功';
            logConnection.classList.add('connected');
            logConnectionText.textContent = 'UE5 已连接';
            if (emptyStateTitle) emptyStateTitle.textContent = '✅ UE5 链接成功';
            if (emptyStateSubtitle) emptyStateSubtitle.textContent = '等待你发送任务，消息将实时显示在这里';
        } else {
            statusDot.className = 'dot disconnected';
            statusText.textContent = `Server Running on :${port} | 等待 UE5 连接...`;
            logConnection.classList.remove('connected');
            logConnectionText.textContent = '等待 UE5 连接';
            if (emptyStateTitle) emptyStateTitle.textContent = '等待 UE5 仿真端连接...';
            if (emptyStateSubtitle) emptyStateSubtitle.textContent = '消息将实时显示在这里';
        }
        if (connected && !previous) addMessage('连接状态', 'UE5 仿真端', '链接成功，可以发送任务并接收回传。', 'system');
        else if (!connected && previous) addMessage('连接状态', 'UE5 仿真端', '连接已断开，等待 UE5 重新轮询。', 'system', 'error');
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
            addMessage('待审核计划', taskGraphs[taskKey]?.name || taskKey, await response.json(), 'outgoing');
        } catch (error) {
            addMessage('待审核计划', taskGraphs[taskKey]?.name || taskKey, `发送失败: ${error.message}`, 'system', 'error');
        }
    }

    async function sendSkillAllocation(allocationKey) {
        try {
            const response = await fetch('/api/send_skill_allocation', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ allocation_key: allocationKey })
            });
            addMessage('待审核排班', skillAllocations[allocationKey]?.name || allocationKey, await response.json(), 'outgoing');
        } catch (error) {
            addMessage('待审核排班', skillAllocations[allocationKey]?.name || allocationKey, `发送失败: ${error.message}`, 'system', 'error');
        }
    }

    async function sendSkill(skillKey) {
        try {
            const response = await fetch('/api/send_skill', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ skill_key: skillKey })
            });
            addMessage('直接执行示例', skillLists[skillKey]?.name || skillKey, await response.json(), 'outgoing');
        } catch (error) {
            addMessage('直接执行示例', skillLists[skillKey]?.name || skillKey, `发送失败: ${error.message}`, 'system', 'error');
        }
    }

    async function sendRequestUserCommand() {
        try {
            const response = await fetch('/api/send_request_user_command', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({})
            });
            addMessage('辅助工具', '索要用户指令', await response.json(), 'outgoing');
        } catch (error) {
            addMessage('辅助工具', '索要用户指令', `发送失败: ${error.message}`, 'system', 'error');
        }
    }

    function openDemoPage() {
        addMessage('完整流程 Demo', '打开引导演示页', '切换到 /demo，引导式串联审核计划、审核排班和最终执行。', 'system');
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
                hint: '点击卡片查看 DAG，点击按钮发送到 HITL 计划审核。',
                accentClass: 'plan',
                actionLabel: '发送审核',
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
                hint: '点击卡片查看甘特图，点击按钮发送到 HITL 排班审核。',
                accentClass: 'allocation',
                actionLabel: '发送审核',
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
                hint: '点击卡片查看执行时间线，点击按钮直接下发到 Platform。',
                accentClass: 'execute',
                actionLabel: '直接执行',
                onAction: () => sendSkill(key)
            }));
        });

        demoFlowButtonsDiv.appendChild(buildPreviewItem({
            type: 'demo_flow',
            key: 'guided_demo',
            title: '🎬 引导演示页',
            description: '按步骤体验计划审核、排班审核和最终执行，不直接在这里下发任务。',
            meta: ['教学模式', '逐步操作', '端到端体验'],
            hint: '点击卡片先看完整流程图，点击按钮进入 /demo。',
            accentClass: 'demo',
            actionLabel: '进入 Demo',
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
            addMessage(payload.type || 'UE 回传', payload.title || '未命名消息', payload.content, payload.direction || 'incoming');
        };
        evtSource.onerror = () => {
            applyUEConnectionStatus(false);
            statusText.textContent = `Server Running on :${port} | 连接丢失，等待重连...`;
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
        statusText.textContent = `Server Running on :${port} | 等待 UE5 连接...`;
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
