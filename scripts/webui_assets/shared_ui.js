(function () {
    const mermaidConfig = {
        startOnLoad: false,
        theme: 'base',
        themeVariables: {
            darkMode: false,
            background: '#ffffff',
            primaryColor: '#e0f2fe',
            primaryTextColor: '#102033',
            primaryBorderColor: '#7dd3fc',
            lineColor: '#0284c7',
            secondaryColor: '#fff4db',
            tertiaryColor: '#f8fbff',
            clusterBkg: '#ffffff',
            clusterBorder: '#cbd5e1',
            edgeLabelBackground: '#f8fbff',
            actorBorder: '#7dd3fc',
            actorBkg: '#ffffff',
            actorTextColor: '#102033',
            noteBkgColor: '#f8fbff',
            noteBorderColor: '#cbd5e1',
            noteTextColor: '#334155'
        }
    };

    function escapeHtml(text) {
        return String(text ?? '').replace(/[&<>"']/g, (ch) => ({
            '&': '&amp;',
            '<': '&lt;',
            '>': '&gt;',
            '"': '&quot;',
            "'": '&#39;'
        }[ch]));
    }

    function humanizeSkill(skill) {
        return String(skill ?? 'skill').replace(/_/g, ' ');
    }

    function sortNumericKeys(keys) {
        return [...keys].sort((a, b) => Number(a) - Number(b));
    }

    function buildTimelineModel(data) {
        const safeData = data && typeof data === 'object' ? data : {};
        const timeSteps = sortNumericKeys(Object.keys(safeData));
        const rowMap = new Map();
        let maxStep = 0;

        timeSteps.forEach((timeStepKey) => {
            const timeStep = Number(timeStepKey);
            const entries = safeData[timeStepKey] || {};
            Object.entries(entries).forEach(([robotId, command]) => {
                if (!rowMap.has(robotId)) rowMap.set(robotId, []);
                const start = Number.isFinite(timeStep) ? timeStep : 0;
                const end = start + 1;
                rowMap.get(robotId).push({
                    robotId,
                    skill: String(command?.skill || 'skill'),
                    label: humanizeSkill(command?.skill || 'skill'),
                    start,
                    end,
                    params: command?.params || {}
                });
                maxStep = Math.max(maxStep, end);
            });
        });

        const rows = [...rowMap.entries()].map(([robotId, tasks]) => ({
            robotId,
            tasks: tasks.sort((a, b) => a.start - b.start)
        }));

        return {
            rows,
            rowCount: rows.length,
            timeSteps,
            columnCount: Math.max(maxStep, 1)
        };
    }

    function buildTimelineMeta(data, reviewLabel) {
        const model = buildTimelineModel(data);
        return [
            `${model.columnCount} time steps`,
            `${model.rowCount} robots`,
            reviewLabel
        ];
    }

    function ensureMermaid() {
        if (!window.mermaid || typeof window.mermaid.initialize !== 'function') return false;
        if (!window.__multiAgentMermaidReady) {
            window.mermaid.initialize(mermaidConfig);
            window.__multiAgentMermaidReady = true;
        }
        return true;
    }

    function renderMermaidInto(container, code) {
        if (!container) return false;
        if (!code) {
            container.innerHTML = '';
            return false;
        }
        if (!ensureMermaid()) {
            return false;
        }
        const pre = document.createElement('pre');
        pre.className = 'mermaid';
        pre.textContent = code;
        container.innerHTML = '';
        container.appendChild(pre);
        pre.removeAttribute('data-processed');
        window.mermaid.run({ nodes: [pre] });
        return true;
    }

    function summarizeContent(content) {
        if (content == null) return 'No summary';
        if (typeof content === 'string') {
            return content.length > 180 ? `${content.slice(0, 177)}...` : content;
        }
        if (typeof content !== 'object') return String(content);

        const parts = [];
        if (content.status) parts.push(`status: ${content.status}`);
        if (content.endpoint) parts.push(`endpoint: ${content.endpoint}`);
        if (content.note) parts.push(content.note);
        if (content.error) parts.push(`error: ${content.error}`);
        if (content.message) parts.push(content.message);
        if (content.result) parts.push(`result: ${content.result}`);
        if (content.review_result) parts.push(`review: ${content.review_result}`);
        if (typeof content.approved === 'boolean') parts.push(content.approved ? 'approved' : 'rejected');
        if (content.feedback) parts.push(`feedback: ${content.feedback}`);
        if (content.comment) parts.push(`comment: ${content.comment}`);
        if (content.scene) parts.push(`scene: ${content.scene}`);
        if (content.change_type) parts.push(`change: ${content.change_type}`);
        if (content.message_type) parts.push(`message_type: ${content.message_type}`);
        if (parts.length) return parts.join(' | ');

        const compact = JSON.stringify(content);
        return compact.length > 180 ? `${compact.slice(0, 177)}...` : compact;
    }

    function renderTimelineGantt(container, options) {
        if (!container) return false;
        const model = buildTimelineModel(options?.data || {});
        if (!model.rowCount) {
            container.innerHTML = '<div class="ma-gantt-empty"><strong>No timeline data</strong><span>The current entry has no skill allocation to visualize.</span></div>';
            return false;
        }

        const sidebarWidth = Number(options?.sidebarWidth || 220);
        const columnWidth = Number(options?.columnWidth || 140);
        const title = escapeHtml(options?.title || 'Skill Allocation Timeline');
        const subtitle = escapeHtml(options?.subtitle || `${model.columnCount} time steps · ${model.rowCount} robots`);
        const badges = Array.isArray(options?.badges) ? options.badges : [];
        const badgeHtml = badges.map((badge) => `<span class="ma-gantt-badge">${escapeHtml(badge)}</span>`).join('');
        const axisHtml = Array.from({ length: model.columnCount }, (_, index) => `
                <div class="ma-gantt-axis-cell">
                <div class="ma-gantt-axis-label">Step</div>
                <div class="ma-gantt-axis-value">${index}</div>
            </div>
        `).join('');
        const leftRowsHtml = model.rows.map((row) => `
            <div class="ma-gantt-robot-row">
                <span class="ma-gantt-robot-name">${escapeHtml(row.robotId)}</span>
                <span class="ma-gantt-robot-meta">${row.tasks.length} skills</span>
            </div>
        `).join('');
        const rightRowsHtml = model.rows.map((row) => {
            const cells = Array.from({ length: model.columnCount }, () => '<div class="ma-gantt-grid-cell"></div>').join('');
            const bars = row.tasks.map((task) => `
                <div class="ma-gantt-bar" style="grid-column: ${task.start + 1} / ${task.end + 1};">
                    <span class="ma-gantt-bar-label">${escapeHtml(task.label)}</span>
                    <span class="ma-gantt-bar-range">${task.start} → ${task.end}</span>
                </div>
            `).join('');
            return `<div class="ma-gantt-grid-row">${cells}${bars}</div>`;
        }).join('');

        container.innerHTML = `
            <div class="ma-gantt" style="--gantt-sidebar-width:${sidebarWidth}px; --gantt-column-width:${columnWidth}px; --gantt-columns:${model.columnCount};">
                <div class="ma-gantt-header">
                    <div class="ma-gantt-title-wrap">
                        <div class="ma-gantt-title">${title}</div>
                        <div class="ma-gantt-subtitle">${subtitle}</div>
                    </div>
                    <div class="ma-gantt-badges">${badgeHtml}</div>
                </div>
                <div class="ma-gantt-shell">
                    <div class="ma-gantt-left">
                        <div class="ma-gantt-corner">Robots</div>
                        <div class="ma-gantt-robot-list">${leftRowsHtml}</div>
                    </div>
                    <div class="ma-gantt-right">
                        <div class="ma-gantt-axis">${axisHtml}</div>
                        <div class="ma-gantt-grid">${rightRowsHtml}</div>
                    </div>
                </div>
            </div>
        `;
        return true;
    }

    window.MultiAgentUI = {
        escapeHtml,
        humanizeSkill,
        buildTimelineModel,
        buildTimelineMeta,
        ensureMermaid,
        renderMermaidInto,
        summarizeContent,
        renderTimelineGantt
    };
})();
