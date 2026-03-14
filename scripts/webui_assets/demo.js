(function () {
    const UI = window.MultiAgentUI;
    const boot = window.MA_DEMO_DATA || {};
    const demoScenarios = boot.scenarios || {};
    const skillLists = boot.skillLists || {};
    const skillAllocations = boot.skillAllocations || {};
    const demoColors = ['#00d9ff', '#e94560', '#ffd700'];
    let demoCur = null;
    let demoStep = 0;
    let demoSent = new Set();

    function demoFmtTip(text) {
        return String(text || '')
            .replace(/(Tab|Ctrl|Space|Shift|Confirm|Reject|Edit)/g, '<kbd>$1</kbd>')
            .replace(/(按)([A-Z0-9,]{1,3})/g, '$1 <kbd>$2</kbd>');
    }

    function buildDemoCards() {
        const box = document.getElementById('demo-cards');
        let colorIndex = 0;
        Object.entries(demoScenarios).forEach(([key, scenario]) => {
            const color = demoColors[colorIndex % demoColors.length];
            const stars = Array.from({ length: 3 }, (_, idx) => (
                `<span class="demo-star${idx < scenario.difficulty ? '' : ' off'}">★</span>`
            )).join('');
            const robots = (scenario.robot_types || []).map((robot) => `<span>${UI.escapeHtml(robot)}</span>`).join('');
            const card = document.createElement('div');
            card.className = 'demo-card';
            card.style.setProperty('--c', color);
            card.onclick = () => demoEnter(key);
            card.innerHTML = `
                <div class="demo-card-icon">${scenario.icon}</div>
                <div class="demo-card-title">${UI.escapeHtml(scenario.name)}</div>
                <div class="demo-card-diff">${stars}<span style="margin-left:6px;color:var(--muted);">${UI.escapeHtml(scenario.difficulty_label)}</span></div>
                <div class="demo-card-desc">${UI.escapeHtml(scenario.description)}</div>
                <div class="demo-card-robots">${robots}</div>
                <div class="demo-card-enter">开始体验 →</div>
            `;
            box.appendChild(card);
            colorIndex += 1;
        });
    }

    function demoEnter(key) {
        demoCur = demoScenarios[key];
        demoStep = 0;
        demoSent = new Set();
        document.getElementById('demo-landing').style.display = 'none';
        document.getElementById('demo-guide').classList.add('active');
        document.getElementById('dg-title').textContent = `${demoCur.icon} ${demoCur.name}`;
        const progress = document.getElementById('dg-progress');
        progress.innerHTML = '';
        demoCur.steps.forEach((_, index) => {
            const bar = document.createElement('div');
            bar.className = 'pbar';
            bar.id = `dpb-${index}`;
            progress.appendChild(bar);
        });
        demoRender();
    }

    function demoBack() {
        document.getElementById('demo-guide').classList.remove('active');
        document.getElementById('demo-landing').style.display = '';
        demoCur = null;
        document.getElementById('demo-sb').innerHTML = '<div class="demo-sidebar-empty">发送命令后，日志将显示在这里</div>';
    }

    function lookupVisualizationData(visualization) {
        if (!visualization) return null;
        if (visualization.source === 'skill_allocation') return skillAllocations[visualization.key]?.data || null;
        if (visualization.source === 'skill_list') return skillLists[visualization.key]?.data || null;
        return null;
    }

    function renderVisualization(step) {
        const visual = document.getElementById('ds-visual');
        if (!demoCur || step !== 0) {
            visual.style.display = 'none';
            visual.innerHTML = '';
            return;
        }

        const visualization = demoCur.visualization;
        if (!visualization) {
            visual.style.display = 'none';
            visual.innerHTML = '';
            return;
        }

        visual.style.display = '';
        if (visualization.kind === 'timeline') {
            const timelineData = lookupVisualizationData(visualization);
            if (timelineData) {
                UI.renderTimelineGantt(visual, {
                    data: timelineData,
                    title: visualization.title || demoCur.name,
                    subtitle: visualization.subtitle || demoCur.description,
                    badges: UI.buildTimelineMeta(timelineData, visualization.badge || 'demo'),
                    sidebarWidth: 220,
                    columnWidth: 132
                });
                return;
            }
        }

        const mermaidCode = visualization.mermaid || demoCur.mermaid;
        if (!mermaidCode) {
            visual.style.display = 'none';
            visual.innerHTML = '';
            return;
        }
        const rendered = UI.renderMermaidInto(visual, mermaidCode);
        if (!rendered) {
            visual.innerHTML = '<div class="ma-gantt-empty"><strong>可视化引擎加载中</strong><span>Mermaid 初始化后会自动刷新。</span></div>';
        }
    }

    function demoRender() {
        if (!demoCur) return;
        const step = demoCur.steps[demoStep];
        const total = demoCur.steps.length;
        for (let i = 0; i < total; i += 1) {
            document.getElementById(`dpb-${i}`).className = `pbar${i < demoStep ? ' done' : i === demoStep ? ' now' : ''}`;
        }
        document.getElementById('ds-num').textContent = `STEP ${demoStep + 1} / ${total}`;
        document.getElementById('dg-sub').textContent = `${demoStep + 1} / ${total}`;
        document.getElementById('ds-title').textContent = step.title;
        document.getElementById('ds-desc').textContent = step.description;
        document.getElementById('ds-uetip').innerHTML = demoFmtTip(step.ue_tip);
        renderVisualization(demoStep);

        const actionArea = document.getElementById('ds-actions');
        actionArea.innerHTML = '';
        if (step.action_type !== 'info') {
            const done = demoSent.has(demoStep);
            const btn = document.createElement('button');
            btn.className = 'act-btn primary';
            btn.disabled = done;
            btn.textContent = done ? '✅ 已发送' : `🚀 ${step.action_label}`;
            btn.onclick = () => demoSend(demoStep, step);
            actionArea.appendChild(btn);
        } else {
            const hint = document.createElement('span');
            hint.style.cssText = 'color: var(--muted-soft); font-size: 14px;';
            hint.textContent = step.action_label;
            actionArea.appendChild(hint);
        }

        document.getElementById('dn-prev').disabled = demoStep === 0;
        const nextBtn = document.getElementById('dn-next');
        if (demoStep >= total - 1) {
            nextBtn.textContent = '完成 ✓';
            nextBtn.onclick = () => demoBack();
        } else {
            nextBtn.textContent = '下一步 →';
            nextBtn.disabled = false;
            nextBtn.onclick = () => demoNav(1);
        }
    }

    function demoNav(direction) {
        if (!demoCur) return;
        const next = demoStep + direction;
        if (next < 0 || next >= demoCur.steps.length) return;
        demoStep = next;
        demoRender();
    }

    async function demoSend(stepIndex, step) {
        try {
            const response = await fetch('/api/demo/send_step', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ action_type: step.action_type, action_key: step.action_key })
            });
            const payload = await response.json();
            demoSent.add(stepIndex);
            demoRender();
            demoAddLog(step.action_label, payload, '📤 Backend → UE5');
        } catch (error) {
            demoAddLog('Error', { error: error.message }, '⚠️ Demo Error');
        }
    }

    function demoAddLog(title, content, directionLabel) {
        const sidebar = document.getElementById('demo-sb');
        const empty = sidebar.querySelector('.demo-sidebar-empty');
        if (empty) empty.remove();
        const item = document.createElement('div');
        item.className = 'demo-msg';
        item.innerHTML = `
            <div class="dir">${directionLabel}</div>
            <div class="hdr"><span class="badge">${UI.escapeHtml(title)}</span><span class="time">${new Date().toLocaleTimeString()}</span></div>
            <div class="body">${UI.escapeHtml(UI.summarizeContent(content))}</div>
        `;
        sidebar.insertBefore(item, sidebar.firstChild);
    }

    function init() {
        buildDemoCards();
        window.demoBack = demoBack;
        window.demoNav = demoNav;
    }

    window.addEventListener('DOMContentLoaded', init);
})();
