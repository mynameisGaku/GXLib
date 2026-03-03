// app.js — GXLib API Reference SPA
(function() {
"use strict";

// ====================================================================
// Data access
// ====================================================================

var DATA = window.GX_API_DATA || { modules: {}, files: {}, types: [] };

// ====================================================================
// DOM refs
// ====================================================================

var $sidebar    = document.getElementById("sidebar");
var $sidebarNav = document.getElementById("sidebar-nav");
var $overlay    = document.getElementById("sidebar-overlay");
var $menuToggle = document.getElementById("menu-toggle");
var $searchInput= document.getElementById("search-input");
var $searchBox  = document.getElementById("search-box");
var $searchRes  = document.getElementById("search-results");
var $breadcrumb = document.getElementById("breadcrumb");
var $content    = document.getElementById("content");
var $stats      = document.getElementById("stats");
var $logo       = document.querySelector(".logo");

// ====================================================================
// Stats
// ====================================================================

function computeStats() {
    var mods = Object.keys(DATA.modules).length;
    var files = Object.keys(DATA.files).length;
    var types = DATA.types.length;
    var methods = 0;
    DATA.types.forEach(function(t) {
        if (t.methods) methods += t.methods.length;
    });
    $stats.innerHTML = "<b>" + mods + "</b> モジュール &middot; <b>" + files +
        "</b> ファイル &middot; <b>" + types + "</b> 型 &middot; <b>" + methods + "</b> メソッド";
}

// ====================================================================
// Sidebar
// ====================================================================

function buildSidebar() {
    var html = "";
    var moduleIds = Object.keys(DATA.modules).sort(function(a,b) {
        return DATA.modules[a].name.localeCompare(DATA.modules[b].name);
    });

    moduleIds.forEach(function(mid) {
        var mod = DATA.modules[mid];
        var filePaths = Object.keys(mod.files).sort();
        var fileCount = filePaths.length;

        html += '<div class="sb-module" data-mid="' + mid + '">';
        html += '<div class="sb-module-head" onclick="App.openModule(\'' + mid + '\')">';
        html += '<span class="sb-module-arrow" onclick="event.stopPropagation();App.toggleModule(\'' + mid + '\')">&#9654;</span>';
        html += '<span class="sb-module-icon">' + mod.icon + '</span>';
        html += '<span class="sb-module-name">' + esc(mod.name) + '</span>';
        html += '<span class="sb-module-count">' + fileCount + '</span>';
        html += '</div>';
        html += '<div class="sb-files">';

        filePaths.forEach(function(fp) {
            var fname = mod.files[fp];
            var f = DATA.files[fp];
            var typeItems = [];
            if (f && f.items) {
                typeItems = f.items.filter(function(it) {
                    return it.kind === "class" || it.kind === "struct" || it.kind === "enum" || it.kind === "constns";
                });
            }
            var hasItems = typeItems.length > 0;

            // File entry — collapsible if it has type items
            html += '<div class="sb-file-group" data-fp="' + esc(fp) + '">';
            html += '<div class="sb-file' + (hasItems ? " sb-file-parent" : "") + '" data-fp="' + esc(fp) + '">';
            if (hasItems) html += '<span class="sb-file-arrow">&#9654;</span>';
            html += '<span class="sb-file-name" onclick="App.navigateTo(\'file\',\'' + escAttr(fp) + '\')">' + esc(fname) + '</span>';
            if (hasItems) html += '<span class="sb-file-count">' + typeItems.length + '</span>';
            html += '</div>';

            if (hasItems) {
                html += '<div class="sb-items">';
                typeItems.forEach(function(it) {
                    var badge = badgeClass(it.kind);
                    html += '<div class="sb-type-item" onclick="App.navigateTo(\'type\',' + it.idx + ')">';
                    html += '<span class="badge ' + badge + '">' + it.kind + '</span>';
                    html += '<span>' + esc(it.name) + '</span>';
                    html += '</div>';
                });
                html += '</div>';
            }
            html += '</div>';
        });

        html += '</div></div>';
    });

    $sidebarNav.innerHTML = html;

    // File-level toggle (click the file row to expand/collapse items)
    $sidebarNav.querySelectorAll(".sb-file-parent").forEach(function(el) {
        el.addEventListener("click", function(e) {
            // Don't toggle if user clicked the file name link itself
            if (e.target.classList.contains("sb-file-name")) return;
            var group = el.closest(".sb-file-group");
            if (group) group.classList.toggle("expanded");
        });
    });
}

function badgeClass(kind) {
    switch(kind) {
        case "class":   return "badge-class";
        case "struct":  return "badge-struct";
        case "enum":    return "badge-enum";
        case "constns": return "badge-constns";
        case "func":    return "badge-func";
        case "typedef": case "using": return "badge-typedef";
        default: return "";
    }
}

// ====================================================================
// Navigation
// ====================================================================

function navigateTo(kind, id) {
    closeSidebar();
    closeSearch();
    closeAllTooltips();

    if (kind === "home") {
        renderLanding();
        updateHash("");
        setBreadcrumb([]);
        clearSidebarActive();
    } else if (kind === "module") {
        renderModuleOverview(id);
        updateHash("module/" + id);
        setBreadcrumb([{ text: DATA.modules[id] ? DATA.modules[id].name : id }]);
        setSidebarModuleActive(id);
    } else if (kind === "file") {
        renderFileView(id);
        var f = DATA.files[id];
        var mod = f ? DATA.modules[f.module] : null;
        var fname = id.split("/").pop();
        var bc = [];
        if (mod) bc.push({ text: mod.name, action: "App.navigateTo('module','" + f.module + "')" });
        bc.push({ text: fname });
        setBreadcrumb(bc);
        updateHash("file/" + id);
        setSidebarFileActive(id);
    } else if (kind === "type") {
        var idx = typeof id === "string" ? parseInt(id) : id;
        var t = DATA.types[idx];
        if (!t) return;
        renderTypeView(t, idx);
        var f2 = DATA.files[t.file];
        var mod2 = f2 ? DATA.modules[f2.module] : null;
        var fname2 = t.file.split("/").pop();
        var bc2 = [];
        if (mod2) bc2.push({ text: mod2.name, action: "App.navigateTo('module','" + t.module + "')" });
        bc2.push({ text: fname2, action: "App.navigateTo('file','" + escAttr(t.file) + "')" });
        bc2.push({ text: t.name });
        setBreadcrumb(bc2);
        updateHash("type/" + idx);
        setSidebarFileActive(t.file);
    } else if (kind === "glossary") {
        renderGlossaryPage(id || "");
        updateHash("glossary" + (id ? "/" + id : ""));
        setBreadcrumb([{ text: "用語辞典" }]);
        clearSidebarActive();
    } else if (kind === "classdict") {
        renderClassDictPage(id || "");
        updateHash("classdict" + (id ? "/" + id : ""));
        setBreadcrumb([{ text: "クラス辞典" }]);
        clearSidebarActive();
    }

    // Scroll to top
    document.getElementById("main").scrollTop = 0;
}

var _isInitialLoad = true;
function updateHash(h) {
    var newHash = h ? "#/" + h : window.location.pathname;
    if (_isInitialLoad || window.location.hash === newHash) {
        history.replaceState(null, "", newHash);
        _isInitialLoad = false;
    } else {
        history.pushState(null, "", newHash);
    }
}

function setBreadcrumb(items) {
    if (items.length === 0) {
        $breadcrumb.innerHTML = "";
        return;
    }
    var html = '<a href="#" onclick="App.navigateTo(\'home\');return false;">ホーム</a>';
    items.forEach(function(item, i) {
        html += ' <span class="bc-sep">&#9654;</span> ';
        if (i < items.length - 1 && item.action) {
            html += '<a href="#" onclick="' + item.action + ';return false;">' + esc(item.text) + '</a>';
        } else {
            html += '<span class="bc-current">' + esc(item.text) + '</span>';
        }
    });
    $breadcrumb.innerHTML = html;
}

function clearSidebarActive() {
    $sidebarNav.querySelectorAll(".active").forEach(function(el) { el.classList.remove("active"); });
}

function setSidebarModuleActive(mid) {
    clearSidebarActive();
    var el = $sidebarNav.querySelector('.sb-module[data-mid="' + mid + '"] .sb-module-head');
    if (el) el.classList.add("active");
    expandModule(mid);
}

function setSidebarFileActive(fp) {
    clearSidebarActive();
    var fileEl = $sidebarNav.querySelector('.sb-file[data-fp="' + CSS.escape(fp) + '"]');
    if (fileEl) {
        fileEl.classList.add("active");
        // Expand parent module
        var modEl = fileEl.closest(".sb-module");
        if (modEl) modEl.classList.add("expanded");
        // Expand file group to show items
        var groupEl = fileEl.closest(".sb-file-group");
        if (groupEl) groupEl.classList.add("expanded");
    }
}

function expandModule(mid) {
    var el = $sidebarNav.querySelector('.sb-module[data-mid="' + mid + '"]');
    if (el) el.classList.add("expanded");
}

function toggleModule(mid) {
    var el = $sidebarNav.querySelector('.sb-module[data-mid="' + mid + '"]');
    if (el) {
        if (el.classList.contains("expanded")) {
            el.classList.remove("expanded");
            // Also hide all sub-items
            el.querySelectorAll(".sb-items").forEach(function(si) { si.style.display = ""; });
        } else {
            el.classList.add("expanded");
        }
    }
}

function openModule(mid) {
    navigateTo('module', mid);
}

// ====================================================================
// Render: Landing Page
// ====================================================================

function renderLanding() {
    var html = '<div class="landing-header">';
    html += '<h1>GXLib APIリファレンス</h1>';
    html += '<p>DirectX 12 ゲームエンジン — ' + Object.keys(DATA.files).length + ' ヘッダー / ' +
            DATA.types.length + ' 型 / ' + Object.keys(DATA.modules).length + ' モジュール</p>';
    html += '</div>';

    // Dictionary cards at the top
    var termCount = Object.keys(GLOSSARY).length;
    var classCount = DATA.types.length;

    html += '<div class="dict-cards">';
    html += '<div class="dict-card dict-card-class" onclick="App.navigateTo(\'classdict\')">';
    html += '<div class="dict-card-icon">📘</div>';
    html += '<div class="dict-card-body">';
    html += '<div class="dict-card-title">クラス辞典</div>';
    html += '<div class="dict-card-desc">全 ' + classCount + ' 件のクラス・構造体・列挙型をアルファベット順に参照</div>';
    html += '</div></div>';
    html += '<div class="dict-card dict-card-term" onclick="App.navigateTo(\'glossary\')">';
    html += '<div class="dict-card-icon">📖</div>';
    html += '<div class="dict-card-body">';
    html += '<div class="dict-card-title">用語辞典</div>';
    html += '<div class="dict-card-desc">全 ' + termCount + ' 件のC++・グラフィックス・ゲームエンジン用語</div>';
    html += '</div></div>';
    html += '</div>';

    html += '<div class="module-grid">';

    var mids = Object.keys(DATA.modules).sort(function(a,b) {
        return DATA.modules[a].name.localeCompare(DATA.modules[b].name);
    });

    mids.forEach(function(mid) {
        var mod = DATA.modules[mid];
        var fcount = Object.keys(mod.files).length;
        if (fcount === 0) return;

        var tcount = 0;
        Object.keys(mod.files).forEach(function(fp) {
            var f = DATA.files[fp];
            if (f && f.items) {
                f.items.forEach(function(it) {
                    if (it.kind === "class" || it.kind === "struct" || it.kind === "enum" || it.kind === "constns") tcount++;
                });
            }
        });

        html += '<div class="module-card" onclick="App.navigateTo(\'module\',\'' + mid + '\')">';
        html += '<span class="module-card-icon">' + mod.icon + '</span>';
        html += '<div class="module-card-name">' + esc(mod.name) + '</div>';
        html += '<div class="module-card-desc">' + glossifyText(esc(mod.desc)) + '</div>';
        html += '<div class="module-card-stats"><span><b>' + fcount + '</b> ファイル</span><span><b>' + tcount + '</b> 型</span></div>';
        html += '</div>';
    });

    html += '</div>';

    $content.innerHTML = html;
}

// ====================================================================
// Render: Glossary A-Z Page
// ====================================================================

function renderGlossaryPage(scrollToLetter) {
    // Collect and group glossary entries
    var entries = [];
    for (var key in GLOSSARY) {
        entries.push({ term: key, desc: GLOSSARY[key] });
    }

    // Sort: English terms first (A-Z), then Japanese terms (あ-ん order)
    entries.sort(function(a, b) {
        var aIsEn = /^[a-zA-Z]/.test(a.term);
        var bIsEn = /^[a-zA-Z]/.test(b.term);
        if (aIsEn && !bIsEn) return -1;
        if (!aIsEn && bIsEn) return 1;
        return a.term.localeCompare(b.term, "ja");
    });

    // Group by first letter (English) or kana row (Japanese)
    var kanaRows = {
        "ア": /^[アイウエオ]/,
        "カ": /^[カキクケコガギグゲゴ]/,
        "サ": /^[サシスセソザジズゼゾ]/,
        "タ": /^[タチツテトダヂヅデド]/,
        "ナ": /^[ナニヌネノ]/,
        "ハ": /^[ハヒフヘホバビブベボパピプペポ]/,
        "マ": /^[マミムメモ]/,
        "ヤ": /^[ヤユヨ]/,
        "ラ": /^[ラリルレロ]/,
        "ワ": /^[ワヲン]/
    };
    function getJaGroup(term) {
        // Convert first char to katakana for grouping
        var ch = term.charAt(0);
        var code = ch.charCodeAt(0);
        // Hiragana to Katakana
        if (code >= 0x3041 && code <= 0x3096) ch = String.fromCharCode(code + 0x60);
        for (var row in kanaRows) {
            if (kanaRows[row].test(ch)) return row;
        }
        return "他";
    }

    var groups = {};
    var groupOrder = [];
    entries.forEach(function(e) {
        var isEn = /^[a-zA-Z]/.test(e.term);
        var groupKey = isEn ? e.term.charAt(0).toUpperCase() : getJaGroup(e.term);
        if (!groups[groupKey]) {
            groups[groupKey] = [];
            groupOrder.push(groupKey);
        }
        groups[groupKey].push(e);
    });

    // Build letter navigation
    var html = '<div class="glossary-page">';
    html += '<div class="file-header"><h1>📖 用語辞典</h1>';
    html += '<div class="file-path">C++ / ゲームエンジン / グラフィックス用語集 — ' + entries.length + ' 項目</div>';
    html += '</div>';

    // A-Z jump bar
    html += '<div class="glossary-jumpbar">';
    groupOrder.forEach(function(key) {
        html += '<a class="glossary-jump-link" href="#" onclick="document.getElementById(\'gloss-' +
                esc(key) + '\').scrollIntoView({behavior:\'smooth\',block:\'start\'});return false;">' + esc(key) + '</a>';
    });
    html += '</div>';

    // Render each group
    groupOrder.forEach(function(key) {
        var items = groups[key];
        html += '<div class="glossary-group" id="gloss-' + esc(key) + '">';
        html += '<h2 class="glossary-group-letter">' + esc(key) + ' <span class="glossary-group-count">' + items.length + '</span></h2>';

        items.forEach(function(e) {
            var descHtml = esc(e.desc).replace(/\n/g, "<br>");
            html += '<div class="glossary-entry">';
            html += '<div class="glossary-entry-term">' + esc(e.term) + '</div>';
            html += '<div class="glossary-entry-desc">' + glossifyText(descHtml) + '</div>';
            html += '</div>';
        });

        html += '</div>';
    });

    html += '</div>';
    $content.innerHTML = html;

    // Scroll to specific letter if provided
    if (scrollToLetter) {
        var target = document.getElementById("gloss-" + scrollToLetter);
        if (target) {
            setTimeout(function() { target.scrollIntoView({ behavior: "smooth", block: "start" }); }, 100);
        }
    }
}

// ====================================================================
// Render: Class Dictionary (A-Z)
// ====================================================================

function renderClassDictPage(scrollToLetter) {
    // Collect all types
    var entries = [];
    DATA.types.forEach(function(t, idx) {
        if (!t || !t.name) return;
        entries.push({ name: t.name, kind: t.kind, module: t.module, doc: t.doc || "", idx: idx, ns: t.ns || "" });
    });

    entries.sort(function(a, b) {
        return a.name.localeCompare(b.name, "en", { sensitivity: "base" });
    });

    // Group by first letter
    var groups = {};
    var groupOrder = [];
    entries.forEach(function(e) {
        var firstChar = e.name.charAt(0).toUpperCase();
        if (!/^[A-Z]/.test(firstChar)) firstChar = "#";
        if (!groups[firstChar]) {
            groups[firstChar] = [];
            groupOrder.push(firstChar);
        }
        groups[firstChar].push(e);
    });

    var html = '<div class="glossary-page">';
    html += '<div class="file-header"><h1>📘 クラス辞典</h1>';
    html += '<div class="file-path">全クラス・構造体・列挙型・定数グループ — ' + entries.length + ' 件</div>';
    html += '</div>';

    // Jump bar
    html += '<div class="glossary-jumpbar">';
    groupOrder.forEach(function(key) {
        html += '<a class="glossary-jump-link" href="#" onclick="document.getElementById(\'cdict-' +
                esc(key) + '\').scrollIntoView({behavior:\'smooth\',block:\'start\'});return false;">' + esc(key) + '</a>';
    });
    html += '</div>';

    // Render each group
    groupOrder.forEach(function(key) {
        var items = groups[key];
        html += '<div class="glossary-group" id="cdict-' + esc(key) + '">';
        html += '<h2 class="glossary-group-letter">' + esc(key) + ' <span class="glossary-group-count">' + items.length + '</span></h2>';

        items.forEach(function(e) {
            var modInfo = DATA.modules[e.module];
            var modLabel = modInfo ? modInfo.name : e.module;
            html += '<div class="classdict-entry" onclick="App.navigateTo(\'type\',' + e.idx + ')">';
            html += '<span class="badge ' + badgeClass(e.kind) + '">' + e.kind + '</span>';
            html += '<span class="classdict-name">' + esc(e.name) + '</span>';
            if (e.ns) html += '<span class="classdict-ns">' + esc(e.ns) + '</span>';
            html += '<span class="classdict-module">' + esc(modLabel) + '</span>';
            if (e.doc) html += '<span class="classdict-doc">' + glossifyText(esc(translateDoc(e.doc || ""))) + '</span>';
            html += '</div>';
        });

        html += '</div>';
    });

    html += '</div>';
    $content.innerHTML = html;

    if (scrollToLetter) {
        var target = document.getElementById("cdict-" + scrollToLetter);
        if (target) {
            setTimeout(function() { target.scrollIntoView({ behavior: "smooth", block: "start" }); }, 100);
        }
    }
}

// ====================================================================
// Render: Module Overview
// ====================================================================

function renderModuleOverview(mid) {
    var mod = DATA.modules[mid];
    if (!mod) {
        renderNotFound();
        return;
    }

    var html = '<div class="file-header">';
    html += '<h1><span style="font-size:1.8rem;margin-right:8px;">' + mod.icon + '</span>' + esc(mod.name) + '</h1>';
    html += '<div class="file-path">' + glossifyText(esc(mod.desc)) + '</div>';
    html += '</div>';

    // List all types in this module
    var filePaths = Object.keys(mod.files).sort();

    filePaths.forEach(function(fp) {
        var f = DATA.files[fp];
        var fname = fp.split("/").pop();
        if (!f || !f.items || f.items.length === 0) return;

        var typeItems = f.items.filter(function(it) { return it.idx !== undefined; });
        var funcItems = f.items.filter(function(it) { return it.kind === "func"; });

        if (typeItems.length === 0 && funcItems.length === 0) return;

        html += '<h3 style="color:var(--text-secondary);font-size:0.85rem;margin:16px 0 8px;font-family:var(--font-mono);cursor:pointer;" onclick="App.navigateTo(\'file\',\'' + escAttr(fp) + '\')">' + esc(fname) + '</h3>';
        html += '<div class="type-list">';

        typeItems.forEach(function(it) {
            var t = DATA.types[it.idx];
            if (!t) return;
            html += '<div class="type-list-item" onclick="App.navigateTo(\'type\',' + it.idx + ')">';
            html += '<span class="badge ' + badgeClass(it.kind) + '">' + it.kind + '</span>';
            html += '<span class="tl-name">' + esc(it.name) + '</span>';
            html += '<span class="tl-doc">' + glossifyText(esc(translateDoc(t.doc || ""))) + '</span>';
            html += '</div>';
        });

        funcItems.forEach(function(it) {
            html += '<div class="type-list-item">';
            html += '<span class="badge badge-func">func</span>';
            html += '<span class="tl-name mono" style="font-size:0.8rem;">' + highlightSig(it.sig) + '</span>';
            if (it.doc) html += '<span class="tl-doc">' + glossifyText(esc(translateDoc(it.doc || ""))) + '</span>';
            html += '</div>';
        });

        html += '</div>';
    });

    $content.innerHTML = html;
}

// ====================================================================
// Render: File View
// ====================================================================

function renderFileView(fp) {
    var f = DATA.files[fp];
    if (!f) {
        renderNotFound();
        return;
    }
    var fname = fp.split("/").pop();
    var mod = DATA.modules[f.module];

    var html = '<div class="file-header">';
    html += '<h1>' + esc(fname) + '</h1>';
    html += '<div class="file-path">GXLib/' + esc(fp) + '</div>';
    if (f.ns && f.ns !== "(global)") {
        html += '<div class="file-ns">namespace ' + esc(f.ns) + '</div>';
    }
    html += '</div>';

    if (!f.items || f.items.length === 0) {
        html += '<div class="empty-state"><div class="es-icon">📄</div><h2>公開APIなし</h2><p>このヘッダーには公開された型や関数がありません。</p></div>';
        $content.innerHTML = html;
        return;
    }

    // Types
    var typeItems = f.items.filter(function(it) { return it.idx !== undefined; });
    var funcItems = f.items.filter(function(it) { return it.kind === "func"; });
    var aliasItems = f.items.filter(function(it) { return it.kind === "typedef" || it.kind === "using"; });

    if (typeItems.length > 0) {
        html += '<h2 style="font-size:1rem;color:var(--text-secondary);margin-bottom:10px;">型</h2>';
        html += '<div class="type-list">';
        typeItems.forEach(function(it) {
            var t = DATA.types[it.idx];
            if (!t) return;
            html += '<div class="type-list-item" onclick="App.navigateTo(\'type\',' + it.idx + ')">';
            html += '<span class="badge ' + badgeClass(it.kind) + '">' + it.kind + '</span>';
            html += '<span class="tl-name">' + esc(it.name) + '</span>';
            html += '<span class="tl-doc">' + glossifyText(esc(translateDoc(t.doc || ""))) + '</span>';
            html += '</div>';
        });
        html += '</div>';
    }

    if (funcItems.length > 0) {
        html += '<h2 style="font-size:1rem;color:var(--text-secondary);margin:24px 0 10px;">フリー関数</h2>';
        funcItems.forEach(function(it) {
            html += '<div class="func-item">';
            html += '<div class="func-sig">' + highlightSig(it.sig) + '</div>';
            if (it.doc) html += renderMethodDoc(it.doc);
            html += '</div>';
        });
    }

    if (aliasItems.length > 0) {
        html += '<h2 style="font-size:1rem;color:var(--text-secondary);margin:24px 0 10px;">型エイリアス</h2>';
        aliasItems.forEach(function(it) {
            html += '<div class="func-item">';
            html += '<span class="badge ' + badgeClass(it.kind) + '">' + it.kind + '</span> ';
            html += '<span class="mono" style="font-size:0.82rem;">' + esc(it.sig) + '</span>';
            if (it.doc) html += '<div class="func-doc">' + glossifyText(esc(translateDoc(it.doc || ""))) + '</div>';
            html += '</div>';
        });
    }

    $content.innerHTML = html;
}

// ====================================================================
// Render: Type View (Class/Struct/Enum/ConstNS)
// ====================================================================

function renderTypeView(t, idx) {
    if (t.kind === "enum") {
        renderEnumView(t);
    } else if (t.kind === "constns") {
        renderConstNSView(t);
    } else {
        renderClassView(t);
    }
}

function renderClassView(t) {
    var html = '<div class="file-header">';
    if (t.tpl) html += '<div class="class-tpl">' + esc(t.tpl) + '</div>';
    html += '<h1><span class="badge ' + badgeClass(t.kind) + '">' + t.kind + '</span>' + esc(t.name) + '</h1>';
    if (t.bases) html += '<div class="class-bases">: <b>' + esc(t.bases) + '</b></div>';
    if (t.doc) html += '<div class="class-doc">' + glossifyText(esc(translateDoc(t.doc || ""))) + '</div>';
    html += '<div class="file-path">GXLib/' + esc(t.file) + '</div>';
    if (t.ns && t.ns !== "(global)") {
        html += '<div class="file-ns">namespace ' + esc(t.ns) + '</div>';
    }
    html += '</div>';

    // Group methods by access
    var methodsByAccess = { "public": [], "protected": [], "private": [] };
    (t.methods || []).forEach(function(m) {
        var acc = m.access || "public";
        if (!methodsByAccess[acc]) methodsByAccess[acc] = [];
        methodsByAccess[acc].push(m);
    });

    // Group fields by access
    var fieldsByAccess = { "public": [], "protected": [], "private": [] };
    (t.fields || []).forEach(function(f) {
        var acc = f.access || "public";
        if (!fieldsByAccess[acc]) fieldsByAccess[acc] = [];
        fieldsByAccess[acc].push(f);
    });

    ["public", "protected", "private"].forEach(function(acc) {
        var methods = methodsByAccess[acc];
        var fields = fieldsByAccess[acc];
        if (methods.length === 0 && fields.length === 0) return;

        html += '<div class="class-section">';
        html += '<div class="class-section-title"><span class="access-dot ' + acc + '"></span>';
        html += '<span' + glossaryAttr(acc) + '>' + acc.toUpperCase() + '</span></div>';

        if (fields.length > 0) {
            html += '<div class="subsection-title">フィールド</div>';
            fields.forEach(function(f, i) {
                html += '<div class="member-card">';
                var quals = renderQuals(f.qual);
                if (quals) html += '<div class="member-tags">' + quals + '</div>';
                html += '<div class="member-sig">' + highlightSig(f.sig) + '</div>';
                if (f.doc) html += renderMethodDoc(f.doc);
                html += '</div>';
            });
        }

        if (methods.length > 0) {
            html += '<div class="subsection-title">メソッド</div>';
            methods.forEach(function(m, i) {
                html += '<div class="member-card">';
                var quals = renderQuals(m.qual);
                if (quals) html += '<div class="member-tags">' + quals + '</div>';
                html += '<div class="member-sig">' + highlightSig(m.sig) + '</div>';
                if (m.doc) html += renderMethodDoc(m.doc);
                html += '</div>';
            });
        }

        html += '</div>';
    });

    if ((t.methods || []).length === 0 && (t.fields || []).length === 0) {
        html += '<div class="empty-state"><div class="es-icon">📋</div><h2>メンバーなし</h2><p>この型にはメンバーが見つかりませんでした。</p></div>';
    }

    $content.innerHTML = html;
}

function renderEnumView(t) {
    var html = '<div class="file-header">';
    html += '<h1><span class="badge badge-enum">enum</span>' + esc(t.name) + '</h1>';
    if (t.base) html += '<div class="enum-base">: ' + esc(t.base) + '</div>';
    if (t.doc) html += '<div class="class-doc">' + glossifyText(esc(translateDoc(t.doc || ""))) + '</div>';
    html += '<div class="file-path">GXLib/' + esc(t.file) + '</div>';
    if (t.ns && t.ns !== "(global)") {
        html += '<div class="file-ns">namespace ' + esc(t.ns) + '</div>';
    }
    html += '</div>';

    if (t.values && t.values.length > 0) {
        html += '<table class="enum-table"><thead><tr><th>値</th><th>初期値</th><th>説明</th></tr></thead><tbody>';
        t.values.forEach(function(v) {
            html += '<tr>';
            html += '<td class="ev-name">' + esc(v.name) + '</td>';
            html += '<td class="ev-init">' + esc(v.init || "") + '</td>';
            html += '<td class="ev-doc">' + glossifyText(esc(translateDoc(v.doc || ""))) + '</td>';
            html += '</tr>';
        });
        html += '</tbody></table>';
    } else {
        html += '<div class="empty-state"><div class="es-icon">📋</div><h2>値なし</h2></div>';
    }

    $content.innerHTML = html;
}

function renderConstNSView(t) {
    var html = '<div class="file-header">';
    html += '<h1><span class="badge badge-constns">const</span>' + esc(t.name) + '</h1>';
    if (t.doc) html += '<div class="class-doc">' + glossifyText(esc(translateDoc(t.doc || ""))) + '</div>';
    html += '<div class="file-path">GXLib/' + esc(t.file) + '</div>';
    if (t.ns && t.ns !== "(global)") {
        html += '<div class="file-ns">namespace ' + esc(t.ns) + '</div>';
    }
    html += '</div>';

    if (t.consts && t.consts.length > 0) {
        html += '<table class="const-table"><thead><tr><th>宣言</th><th>説明</th></tr></thead><tbody>';
        t.consts.forEach(function(c) {
            html += '<tr>';
            html += '<td class="ct-sig">' + highlightSig(c.sig) + '</td>';
            html += '<td class="ct-doc">' + glossifyText(esc(translateDoc(c.doc || ""))) + '</td>';
            html += '</tr>';
        });
        html += '</tbody></table>';
    }

    $content.innerHTML = html;
}

function renderNotFound() {
    $content.innerHTML = '<div class="empty-state"><div class="es-icon">🔍</div><h2>見つかりません</h2><p>指定された項目が見つかりませんでした。</p></div>';
}

// ====================================================================
// English → Japanese Doc Translation
// ====================================================================

var EN_DOC_MAP = {
// AI / NavAgent
"Agent that moves along NavMesh paths":"NavMesh経路に沿って移動するエージェント",
"Associate this agent with a NavMesh":"このエージェントをNavMeshに関連付ける",
"Set a destination and compute a path to it":"目的地を設定し経路を計算する",
"Update the agent (move along the path each frame)":"エージェントを更新する（毎フレーム経路に沿って移動）",
"Stop the agent and clear its path":"エージェントを停止し経路をクリアする",
"Get the current world position":"現在のワールド座標を取得する",
"Set the current world position":"現在のワールド座標を設定する",
"Get the current Y-axis rotation (radians)":"現在のY軸回転（ラジアン）を取得する",
"Set the current Y-axis rotation (radians)":"現在のY軸回転（ラジアン）を設定する",
"Check if the agent currently has a valid path":"エージェントが有効な経路を持っているか確認する",
"Check if the agent has reached its destination":"エージェントが目的地に到達したか確認する",
"Get the computed path waypoints":"計算された経路のウェイポイントを取得する",
"Get the current target waypoint index":"現在のターゲットウェイポイントのインデックスを取得する",
"Movement speed (units/sec)":"移動速度（単位/秒）",
"Rotation speed (degrees/sec)":"回転速度（度/秒）",
"Distance to waypoint for advancement":"ウェイポイント到達判定距離",
"Agent Y offset above navmesh surface":"NavMesh表面からのエージェントY方向オフセット",
// AI / NavMesh
"Grid-based navigation mesh":"グリッドベースのナビゲーションメッシュ",
"Build a flat navmesh grid with given world bounds":"指定ワールド範囲でフラットなNavMeshグリッドを構築する",
"Build from a Terrain instance (samples heights automatically)":"Terrainインスタンスから構築する（高さを自動サンプリング）",
"Build from raw geometry (rasterize triangles onto grid)":"生ジオメトリから構築する（三角形をグリッドにラスタライズ）",
"Manually set a cell's walkable state":"セルの歩行可能状態を手動で設定する",
"Set a cell's cost multiplier (e.g. mud = 2.0, water = 3.0)":"セルのコスト倍率を設定する（例: 泥=2.0, 水=3.0）",
"Find a path between two world positions using A*":"A*を使用して2つのワールド座標間の経路を探索する",
"Find the nearest walkable cell to a world position":"ワールド座標に最も近い歩行可能セルを検索する",
"Check if a world position is on a walkable cell":"ワールド座標が歩行可能セル上にあるか確認する",
"Debug draw using PrimitiveBatch3D (green = walkable, red = blocked)":"PrimitiveBatch3Dでデバッグ描画する（緑=歩行可能, 赤=ブロック）",
"Debug draw a path as a series of lines":"経路を一連の線としてデバッグ描画する",
// Touch / Gesture
"Phase of a touch point lifecycle":"タッチポイントのライフサイクルフェーズ",
"Touch just started":"タッチ開始",
"Touch position changed":"タッチ位置が変更された",
"Touch is active but not moving":"タッチ中だが移動していない",
"Touch was released":"タッチが離された",
"Touch was cancelled (e.g., system interrupt)":"タッチがキャンセルされた（例: システム割り込み）",
"Data for a single touch point":"単一タッチポイントのデータ",
"Current position":"現在の位置",
"Position last frame":"前フレームの位置",
"Position change this frame":"このフレームの位置変化量",
"Pressure (0-1)":"筆圧（0〜1）",
"Touch radius":"タッチ半径",
"Time when this event occurred (seconds)":"このイベントが発生した時刻（秒）",
"Number of taps at this location":"この位置でのタップ回数",
"Type of recognized gesture":"認識されたジェスチャーの種類",
"Direction of a swipe gesture":"スワイプジェスチャーの方向",
"Data for a detected gesture event":"検出されたジェスチャーイベントのデータ",
"Center position":"中心位置",
"Movement delta":"移動量",
"Pinch scale factor":"ピンチスケール係数",
"Rotation angle (radians)":"回転角度（ラジアン）",
"Velocity":"速度",
"Configuration for touch and gesture detection":"タッチおよびジェスチャー検出の設定",
"Max time for a tap (seconds)":"タップの最大時間（秒）",
"Max time between taps for double tap":"ダブルタップ間の最大時間",
"Min time for long press":"長押しの最小時間",
"Min distance for swipe (pixels)":"スワイプの最小距離（ピクセル）",
"Max time for swipe":"スワイプの最大時間",
"Min distance change for pinch":"ピンチの最小距離変化量",
"Touch input manager with multi-touch and gesture detection":"マルチタッチおよびジェスチャー検出対応のタッチ入力マネージャー",
"Set touch configuration":"タッチ設定を設定する",
"Get the current configuration":"現在の設定を取得する",
"Register a new touch":"新しいタッチを登録する",
"Update a touch position":"タッチ位置を更新する",
"End a touch":"タッチを終了する",
"Cancel a touch":"タッチをキャンセルする",
"Process touch states and detect gestures":"タッチ状態を処理しジェスチャーを検出する",
"Get the number of active touches":"アクティブなタッチの数を取得する",
"Get a touch by index":"インデックスでタッチを取得する",
"Get a touch by its unique ID":"一意のIDでタッチを取得する",
"Check if any touch is active":"アクティブなタッチがあるか確認する",
"Get the maximum number of simultaneous touches":"同時タッチの最大数を取得する",
"Get gestures detected this frame":"このフレームで検出されたジェスチャーを取得する",
"Clear detected gestures":"検出されたジェスチャーをクリアする",
"Detect a tap gesture for a touch that just ended":"終了したタッチに対するタップジェスチャーを検出する",
"Detect a double tap gesture":"ダブルタップジェスチャーを検出する",
"Detect a long press gesture":"長押しジェスチャーを検出する",
"Detect a swipe gesture":"スワイプジェスチャーを検出する",
"Detect a pinch gesture (requires 2+ touches)":"ピンチジェスチャーを検出する（2つ以上のタッチが必要）",
"Detect a rotation gesture (requires 2+ touches)":"回転ジェスチャーを検出する（2つ以上のタッチが必要）",
"Enable or disable a gesture type":"ジェスチャータイプを有効/無効にする",
"Check if a gesture type is enabled":"ジェスチャータイプが有効か確認する",
"Get the centroid of all active touches":"全アクティブタッチの重心を取得する",
"Remove all active touches":"全アクティブタッチを削除する",
"Callback invoked when a gesture is detected":"ジェスチャー検出時に呼び出されるコールバック",
"Add a gesture event and invoke callback":"ジェスチャーイベントを追加しコールバックを呼び出す",
"Calculate distance between two points":"2点間の距離を計算する",
"Calculate angle between two points":"2点間の角度を計算する",
// Accessibility
"Semantic role of an accessible element":"アクセシブル要素のセマンティックロール",
"Current state of an accessible element":"アクセシブル要素の現在の状態",
"An element in the accessibility tree":"アクセシビリティツリー内の要素",
"Direction for keyboard focus navigation":"キーボードフォーカスナビゲーションの方向",
"Global accessibility configuration":"グローバルアクセシビリティ設定",
"Yellow":"黄色",
"Pixels":"ピクセル",
"Priority levels for screen reader announcements":"スクリーンリーダー通知の優先度レベル",
"A screen reader announcement event":"スクリーンリーダー通知イベント",
"High contrast color palette":"ハイコントラストカラーパレット",
"Foreground (white)":"前景色（白）",
"Background (black)":"背景色（黒）",
"Accent (cyan)":"アクセントカラー（シアン）",
"Focus indicator (yellow)":"フォーカスインジケーター（黄色）",
"Manager for accessibility features including focus navigation, screen reader announcements, and visual accessibility options":"フォーカスナビゲーション、スクリーンリーダー通知、視覚アクセシビリティオプションを含むアクセシビリティ機能マネージャー",
"Set the accessibility configuration":"アクセシビリティ設定を設定する",
"Register a new accessible element":"新しいアクセシブル要素を登録する",
"Unregister an element":"要素の登録を解除する",
"Get an element by ID":"IDで要素を取得する",
"Get the total number of registered elements":"登録済み要素の総数を取得する",
"Check if an element exists":"要素が存在するか確認する",
"Update an element's state":"要素の状態を更新する",
"Set focus to a specific element":"特定の要素にフォーカスを設定する",
"Get the currently focused element ID":"現在フォーカスされている要素のIDを取得する",
"Check if any element has focus":"フォーカスを持つ要素があるか確認する",
"Navigate focus in the specified direction":"指定方向にフォーカスをナビゲートする",
"Activate (click/press) the focused element":"フォーカスされた要素をアクティベートする（クリック/プレス）",
"Queue a screen reader announcement":"スクリーンリーダー通知をキューに追加する",
"Get all pending announcements":"保留中の全通知を取得する",
"Clear all pending announcements":"保留中の全通知をクリアする",
"Get elements sorted by tab order":"タブオーダー順にソートされた要素を取得する",
"Set the tab order for an element":"要素のタブオーダーを設定する",
"Get all registered elements as a flat list":"登録済みの全要素をフラットリストとして取得する",
"Find elements by label text":"ラベルテキストで要素を検索する",
"Get high contrast color palette based on current config":"現在の設定に基づくハイコントラストカラーパレットを取得する",
"Check if motion should be reduced":"モーション軽減が必要か確認する",
"Get the current font scale factor":"現在のフォントスケール係数を取得する",
"Clear all elements and announcements":"全要素と通知をクリアする",
// DataBinding
"Binding propagation mode":"バインディングの伝播モード",
"Source to target only":"ソースからターゲットへの一方向",
"Both directions":"双方向",
"Initial value only, no subsequent updates":"初期値のみ、以降の更新なし",
"Converter functions for transforming values between source and target":"ソースとターゲット間で値を変換するコンバーター関数",
"Convert source value for UI display":"ソース値をUI表示用に変換する",
"Convert UI value back to source format":"UI値をソース形式に逆変換する",
"An observable property that notifies on value changes":"値変更時に通知するオブザーバブルプロパティ",
"Set the property value and notify observers":"プロパティ値を設定しオブザーバーに通知する",
"Get the current value":"現在の値を取得する",
"Set value without triggering notifications":"通知を発生させずに値を設定する",
"Get the property name":"プロパティ名を取得する",
"Set an integer value":"整数値を設定する",
"Get as integer":"整数値として取得する",
"Set a float value":"浮動小数点値を設定する",
"Get as float":"浮動小数点値として取得する",
"Set a boolean value":"真偽値を設定する",
"Get as boolean":"真偽値として取得する",
"Callback invoked when the value changes. Parameters: (oldValue, newValue)":"値変更時に呼び出されるコールバック。パラメータ: (oldValue, newValue)",
"A collection of named properties that can be observed for changes":"変更を監視可能な名前付きプロパティのコレクション",
"Set a property value (creates if not present)":"プロパティ値を設定する（存在しない場合は作成）",
"Get a property value":"プロパティ値を取得する",
"Check if a property exists":"プロパティが存在するか確認する",
"Remove a property":"プロパティを削除する",
"Get the number of properties":"プロパティの数を取得する",
"Get all property names":"全プロパティ名を取得する",
"Get direct access to a DataProperty object":"DataPropertyオブジェクトに直接アクセスする",
"Callback invoked when any property changes":"いずれかのプロパティ変更時に呼び出されるコールバック",
"A binding definition connecting a source property to a target":"ソースプロパティとターゲットを接続するバインディング定義",
"Manages data bindings between a DataContext and UI targets":"DataContextとUIターゲット間のデータバインディングを管理する",
"Create a new binding":"新しいバインディングを作成する",
"Create a binding with value converter":"値コンバーター付きのバインディングを作成する",
"Remove a binding":"バインディングを削除する",
"Remove all bindings for a specific target":"特定ターゲットの全バインディングを削除する",
"Get the total number of bindings":"バインディングの総数を取得する",
"Get a binding by ID":"IDでバインディングを取得する",
"Set the data context for bindings":"バインディング用のデータコンテキストを設定する",
"Get the current data context":"現在のデータコンテキストを取得する",
"Enable or disable a binding":"バインディングを有効/無効にする",
"Check if a binding is active":"バインディングが有効か確認する",
"Push all source values to targets (process pending changes)":"全ソース値をターゲットにプッシュする（保留中の変更を処理）",
"Notify that a specific property has changed":"特定プロパティの変更を通知する",
"Get all binding IDs for a specific source property":"特定ソースプロパティの全バインディングIDを取得する",
"Get all binding IDs for a specific target":"特定ターゲットの全バインディングIDを取得する",
"Remove all bindings":"全バインディングを削除する",
// VolumetricClouds
"Ray-marched volumetric cloud layer":"レイマーチングによるボリュメトリッククラウドレイヤー",
"Initialise resources, shaders, PSO":"リソース、シェーダー、PSOを初期化する",
"Execute the cloud pass (HDR space)":"クラウドパスを実行する（HDR空間）",
"Handle screen resize":"画面リサイズを処理する",
// ShaderModelGPUParams
"0: albedo RGBA":"0: アルベドRGBA",
"16: emissive RGB":"16: エミッシブRGB",
"36: ShaderModel enum value":"36: ShaderModel列挙値",
"40: MaterialFlags":"40: マテリアルフラグ",
"64:  1st shade color":"64: 第1シェードカラー",
"80:  2nd shade color":"80: 第2シェードカラー",
"96:  base->1st threshold":"96: ベース→第1シェード閾値",
"100: base->1st feather":"100: ベース→第1シェードフェザー",
"104: 1st->2nd threshold":"104: 第1→第2シェード閾値",
"108: 1st->2nd feather":"108: 第1→第2シェードフェザー",
"136: specular power":"136: スペキュラパワー",
"140: specular intensity":"140: スペキュラ強度",
"224: specular color":"224: スペキュラカラー",
"236: CSM shadow influence":"236: CSMシャドウ影響度",
"240: rim shadow mask":"240: リムシャドウマスク",
// Graphics misc
"ClusterInfo[totalClusters]":"ClusterInfo[全クラスター数]",
"uint[totalClusters * maxPerCluster]":"uint[全クラスター数 × クラスターあたり最大数]",
"LightData[maxLights]":"LightData[最大ライト数]",
"512x512 R16G16_FLOAT":"512×512 R16G16_FLOATフォーマット",
"0=Alpha, 1=Additive":"0=アルファ合成, 1=加算合成",
"ST.2084 PQ, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020":"ST.2084 PQ転送関数, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020",
"SDR sRGB":"SDR sRGBモード",
"b1: lightVP for shadow pass":"b1: シャドウパス用ライトビュープロジェクション行列",
"SSR / RT Reflections":"SSR / レイトレーシング反射",
"MinMip + MipRegionUsed":"MinMip + MipRegionUsed対応",
// AssetType
".png, .jpg, .jpeg, .bmp, .tga, .dds, .hdr":"画像: .png, .jpg, .jpeg, .bmp, .tga, .dds, .hdr",
".gxmdl, .fbx, .obj, .gltf, .glb, .gxm":"3Dモデル: .gxmdl, .fbx, .obj, .gltf, .glb, .gxm",
".wav, .mp3":"サウンド: .wav, .mp3",
".ogg":"ストリーミング音声: .ogg",
".ttf, .otf, .spritefont":"フォント: .ttf, .otf, .spritefont",
".hlsl, .hlsli, .cso":"シェーダー: .hlsl, .hlsli, .cso",
".lua":"スクリプト: .lua",
".gxscene":"シーン: .gxscene",
".gxmat":"マテリアル: .gxmat",
".gxanim":"アニメーション: .gxanim",
".gxprefab":"プレハブ: .gxprefab",
".tmx, .tmj":"タイルマップ: .tmx, .tmj",
".json, .xml, .ini":"設定ファイル: .json, .xml, .ini",
// Other
"2^4 sub-divisions per FL":"FLあたり2^4個のサブ分割",
"(oldId, newId)":"(旧ID, 新ID)",
"slotId -> hasConflict":"スロットID → コンフリクト有無",
"HINTERNET":"WinHTTPハンドル (HINTERNET)",
"pitch, yaw, roll (radians)":"ピッチ, ヨー, ロール（ラジアン）"
};

function translateDoc(text) {
    if (!text) return "";
    var t = EN_DOC_MAP[text];
    return t !== undefined ? t : text;
}

// ====================================================================
// Qualifier badges
// ====================================================================

// C++ glossary for beginners — shown as interactive hover tooltips
var GLOSSARY = {
    // ── C++ Keywords & Qualifiers ──
    "virtual":     "仮想関数 — 派生クラスで振る舞いを上書き(オーバーライド)できる関数。\n親クラスのポインタ経由でも、実際の型に応じた関数が呼ばれる（ポリモーフィズム）。",
    "static":      "静的メンバ — クラスのインスタンスを作らなくても使えるメンバ。\nクラス全体で1つだけ共有される。static関数はthisポインタを持たない。",
    "const":       "変更不可 — この関数はオブジェクトの状態を変更しないことを保証する。\n読み取り専用のアクセスに使う。変数に付けると値を変更できなくなる。",
    "constexpr":   "コンパイル時定数 — コンパイル時に値が確定する定数式。\n実行時のオーバーヘッドがゼロになる。constよりも強い制約。",
    "pure":        "純粋仮想関数 — 実装を持たず、派生クラスで必ず実装しなければならない関数。\nこれを持つクラスは抽象クラスとなり、直接インスタンス化できない。= 0 で宣言する。",
    "override":    "オーバーライド — 親クラスの仮想関数を派生クラスで上書きしていることを明示する。\nシグネチャの不一致をコンパイル時に検出できる安全修飾子。",
    "mutable":     "ミュータブル — const関数の中でも変更できるメンバ変数。\nキャッシュやカウンタなど、論理的に「読み取り」でも内部状態を更新する場合に使う。",
    "inline":      "インライン — 関数呼び出しのオーバーヘッドを減らすためのヒント。\nコンパイラが関数の中身を呼び出し元に直接展開する。ヘッダに実装を書く時にも使う。",
    "explicit":    "明示的変換 — 暗黙の型変換を禁止する修飾子。\n意図しない変換によるバグを防ぐ。引数1つのコンストラクタによく付ける。",
    "noexcept":    "例外なし — この関数は例外を投げないことを保証する。\nコンパイラの最適化に役立ち、moveコンストラクタでは特に重要。",
    "volatile":    "揮発性 — コンパイラの最適化を抑制し、毎回メモリから値を読み取る。\nハードウェアレジスタなど外部から変更される変数に使う。",
    "friend":      "フレンド — クラスのprivate/protectedメンバへのアクセスを特定の関数やクラスに許可する。\nカプセル化を一部緩和する仕組み。",
    "final":       "継承禁止 — クラスに付けるとそれ以上継承できなくなる。\n仮想関数に付けると派生クラスでオーバーライドできなくなる。",
    "delete":      "削除済み関数 — = delete で宣言すると、その関数の呼び出しをコンパイルエラーにできる。\nコピー禁止クラスを作る時に使う。",
    "default":     "デフォルト関数 — = default で宣言すると、コンパイラがデフォルト実装を自動生成する。\nコンストラクタやデストラクタに使う。",
    "typename":    "型名指定子 — テンプレートの型パラメータを宣言するキーワード。\ntemplate<typename T>のTが任意の型に置き換わる。classと同じ意味でも使える。",
    "sizeof":      "サイズ演算子 — 型やオブジェクトのメモリ上のバイト数を返す。\nコンパイル時に確定し、実行時コストはゼロ。メモリレイアウトの確認に使う。",
    "alignas":     "アライメント指定 — 変数や型のメモリ配置境界を指定する。\nalignas(16)で16バイト境界に配置。SIMD演算ではアライメントが重要。",
    "decltype":    "型推論 — 式から型を推論するキーワード。\ndecltype(x)はxの型を返す。autoと組み合わせて戻り値型の推論に使う。",
    "auto":        "型推論 — コンパイラが初期化式から型を自動推論する。\nauto x = 42; でint、auto v = vec.begin(); でイテレータ型。可読性とのバランスが大切。",
    "enum class":  "スコープ付き列挙型 — 型安全な列挙型。値がスコープ内に閉じ、暗黙の整数変換を防ぐ。\nenum class Color { Red, Green, Blue }; のようにColor::Redでアクセスする。",
    "struct":      "構造体 — クラスとほぼ同じだが、デフォルトのアクセス修飾子がpublic。\nPOD（Plain Old Data）型やデータの入れ物に使うことが多い。\nC++ではメソッドや継承も可能。",
    "void":        "戻り値なし/汎用ポインタ — 関数の戻り値がないことを示す。\nvoid*は任意の型のポインタを格納できる汎用ポインタ（型安全でないので注意）。",
    "bool":        "真偽値型 — trueまたはfalseの2値を取る型。\n条件分岐やフラグ管理に使う。整数型との暗黙変換がある（0=false、非0=true）。",
    "nullptr":     "ヌルポインタ — 何も指していないポインタを表すリテラル。\n旧来のNULL(0)より型安全。ポインタの初期化や無効状態の表現に使う。",
    // ── Access Specifiers ──
    "public":      "パブリック — クラスの外から自由にアクセスできるメンバ。\nAPIとして外部に公開する関数や変数に使う。",
    "protected":   "プロテクテッド — クラス自身と派生クラスからのみアクセスできるメンバ。\n外部からは見えないが、継承先では使える。",
    "private":     "プライベート — クラス自身からのみアクセスできるメンバ。\n内部実装の詳細を隠蔽し、外部や派生クラスからは見えない。",
    // ── C++ Concepts (English) ──
    "template":    "テンプレート — 型をパラメータ化して汎用的なコードを書く仕組み。\n使う時に具体的な型を指定する（例: vector<int>）。ヘッダに実装を書く必要がある。",
    "namespace":   "名前空間 — 名前の衝突を防ぐためのスコープ。\n同名のクラスや関数を異なる名前空間で共存させられる。gx:: がGXLibの名前空間。",
    "callback":    "コールバック — 特定のイベントが発生した時に呼び出される関数。\nstd::functionや関数ポインタで渡す。「後で呼び返す」関数。",
    "singleton":   "シングルトン — プログラム全体で1つだけインスタンスが存在するデザインパターン。\nInstance()等のstaticメソッドでグローバルにアクセスする。",
    "iterator":    "イテレータ — コンテナの要素を順番にアクセスするためのオブジェクト。\nポインタのように++や*演算子で操作する。begin()〜end()で使う。",
    "RAII":        "リソース取得は初期化（Resource Acquisition Is Initialization）\nコンストラクタでリソースを取得し、デストラクタで解放するC++の基本パターン。\nメモリリークやリソース漏れを防ぐ。スマートポインタはRAIIの代表例。",
    "abstract":    "抽象クラス — 純粋仮想関数(= 0)を持ち、直接インスタンス化できないクラス。\n共通のインターフェースを定義し、派生クラスで具体的な実装を提供する。",
    "polymorphism":"ポリモーフィズム（多態性） — 同じインターフェースで異なる動作を実現する仕組み。\nvirtualを使った動的ポリモーフィズムとtemplateを使った静的ポリモーフィズムがある。",
    "serialize":   "シリアライズ — オブジェクトの状態をバイト列やテキストに変換すること。\nファイル保存やネットワーク送信に使う。逆変換はデシリアライズ。",
    "allocator":   "アロケータ — メモリの確保・解放を管理するオブジェクト。\nカスタムアロケータでメモリ戦略（プール、スタック等）を制御できる。",
    "mutex":       "ミューテックス（相互排他） — 複数スレッドが同時にデータにアクセスするのを防ぐ。\nlock()で排他開始、unlock()で解除。std::lock_guardで自動管理。",
    "descriptor":  "ディスクリプタ — GPUリソース（テクスチャ、バッファ等）への参照情報。\nディスクリプタヒープにまとめて管理し、シェーダーからアクセスする。\nDX12ではCBV/SRV/UAV/サンプラーの4種類のヒープがある。",
    "viewport":    "ビューポート — 描画結果を表示する画面上の矩形領域。\n位置、サイズ、深度範囲を指定する。複数ビューポートで分割画面も可能。",
    "vertex":      "頂点 — 3Dモデルの構成点。位置、法線、UV座標、色などの属性を持つ。\n頂点同士を結んで三角形（ポリゴン）を構成する。",
    "index":       "インデックス — ①配列やバッファの要素番号。②インデックスバッファ：頂点の接続順序を定義し、頂点の重複を避ける。\n同じ頂点を複数の三角形で共有してメモリ効率を上げる。",
    "rasterize":   "ラスタライズ — 3Dの三角形をピクセル（2Dの画素）に変換する処理。\nGPUのパイプラインで頂点シェーダーの後に自動実行される。",
    "culling":     "カリング — 見えないオブジェクトを描画対象から除外する最適化。\n視錐台カリング、オクルージョンカリング、バックフェースカリングなど。",
    "dispatch":    "ディスパッチ — コンピュートシェーダーの実行命令。\nスレッドグループの数を指定してGPU計算を開始する。",
    "swapchain":   "スワップチェーン — 画面表示用のバッファを複数持ち、交互に切り替える仕組み。\n描画中のバッファと表示中のバッファを分離してちらつきを防ぐ。",
    "fence":       "フェンス — CPUとGPUの同期を取るためのオブジェクト。\nGPUの処理完了をCPU側で待機する時に使う。",
    "resource":    "リソース — ①GPUリソース：テクスチャ、バッファ、レンダーターゲットなどGPUメモリ上のデータ。\n②一般的な意味：プログラムが使用するメモリ、ファイルハンドル、ソケットなど。RAIIで安全に管理する。",
    "handle":      "ハンドル — リソースへの間接参照（ID、インデックス、ポインタのラッパー）。\n直接ポインタの代わりにIDを使うことで、リソースの再配置やバリデーションが可能。\nGXLibではテクスチャやサウンドをハンドルで管理する。",
    "pool":        "プール — ①メモリプール：事前確保したメモリブロックから高速に割り当てる仕組み。\n②オブジェクトプール：使い回すオブジェクトを保管する仕組み。弾丸やパーティクルの生成/破棄を高速化する。",
    "batch":       "バッチ — 複数の描画命令やデータをまとめて一括処理すること。\nバッチ処理でドローコール数を減らし、CPU-GPU間の通信を最適化する。\nSpriteBatch、PrimitiveBatchなど。",
    "queue":       "キュー — ①コマンドキュー：GPUへの描画命令を送るための待ち行列。\n②一般的なキュー：FIFO（先入先出）データ構造。タスクやイベントの順序管理に使う。",
    "barrier":     "バリア — GPUリソースの状態遷移を明示する同期命令。\nDX12ではリソースの用途（描画先→シェーダー入力等）を切り替える時に必須。\nバリアの適切な配置がパフォーマンスと正確性の鍵。",
    "transition":  "トランジション — ①リソースバリア：GPUリソースの状態遷移。\n②視覚効果：画面遷移（フェードイン/アウトなど）やUI要素のアニメーション。\n③状態遷移：ステートマシンでの状態変化。",
    "transform":   "トランスフォーム — 位置(translation)、回転(rotation)、拡縮(scale)を組み合わせた座標変換。\n3Dでは4x4行列で表現する。親子関係でローカル座標とワールド座標を変換する。\nGXLibではTransform2D（2D）とTransform3D（3D）がある。",
    "update":      "更新処理 — 毎フレーム呼ばれるロジック処理。\n物理演算、AI、アニメーション、入力処理などをdeltaTimeを使って時間ベースで更新する。\nUpdateとFixedUpdate（固定時間ステップ）を使い分ける場合もある。",
    "render":      "描画処理 — GPUを使って画面に映像を出力する処理。\nUpdateの後に実行され、シーンの状態を元に3D/2Dの映像を生成する。\nフォワードレンダリング、ディファードレンダリングなどの方式がある。",
    "initialize":  "初期化 — オブジェクトやシステムの使用準備。\nGPUデバイスの作成、リソースの読み込み、初期状態の設定などを行う。\nコンストラクタやInitialize()メソッドで実装する。",
    "shutdown":    "終了処理 — システムやリソースの後片付け。\nGPUリソースの解放、ファイルのクローズ、メモリの返却を行う。\nShutdown()やデストラクタで実装。解放順序に注意が必要。",
    "config":      "設定/構成 — システムの動作パラメータ。\n解像度、音量、キーバインド、グラフィックス品質などのユーザー設定やエンジン設定。\nファイル（JSON/INI等）で永続化する。",
    "flag":        "フラグ — ON/OFFの状態を表すビットやbool値。\nビットフラグは複数の状態を1つの整数にまとめられる。\nenum classとビット演算を組み合わせて型安全に使う。",
    "state":       "ステート（状態） — ①オブジェクトの現在の情報の集合。\n②ステートマシン：有限個の状態を切り替えて動作を制御する設計パターン。\nアニメーション、AI、ゲームフローの管理に広く使う。",
    "event":       "イベント — 特定の出来事の発生通知。\n入力イベント（キー押下）、ゲームイベント（敵撃破）、システムイベント（ウィンドウリサイズ）など。\nオブザーバーパターンやコールバックで処理する。",
    "layer":       "レイヤー — ①描画レイヤー：描画の前後関係を制御する仕組み。背景→キャラ→UIの順序。\n②物理レイヤー：衝突判定のグループ分け。「敵の弾は味方に当たらない」などの設定。\n③レンダーレイヤー：合成やマスク処理のための独立した描画面。",
    "clip":        "クリップ — ①アニメーションクリップ：アニメーションデータの1区間。歩き、走り、ジャンプなど。\n②クリッピング：描画範囲を矩形で制限すること。UIのスクロール領域などで使う。\n③オーディオクリップ：音声データの1単位。",
    "blend":       "ブレンド — ①カラーブレンド：色の合成方法。半透明やパーティクルの加算合成に使う。\n②アニメーションブレンド：複数のアニメーションを滑らかに混合する。歩き→走りの遷移など。\n③テクスチャブレンド：複数テクスチャの混合（地形のスプラッティングなど）。",
    "weight":      "ウェイト（重み） — ①ボーンウェイト：頂点が各ボーンからどれだけ影響を受けるかの割合（0〜1）。\n②ブレンドウェイト：アニメーションやテクスチャの混合比率。\n③物理の質量（重さ）としても使われる。",
    "bone":        "ボーン（骨） — スケルタルアニメーションで使う仮想的な骨格要素。\n階層構造（親子関係）を持ち、親ボーンが動くと子ボーンも追従する。\n各頂点はウェイトで複数のボーンに紐付く。",
    "skeleton":    "スケルトン — ボーン（骨）の階層構造全体。3Dキャラクターのアニメーション用骨格。\nルートボーンから末端まで親子関係で接続され、ポーズデータで姿勢を表現する。",
    "animation":   "アニメーション — ①スケルタルアニメーション：ボーンのポーズを時間で変化させてキャラを動かす。\n②スプライトアニメーション：2D画像を切り替えてパラパラ漫画式に動かす。\n③UIアニメーション：ウィジェットの位置、サイズ、透明度の時間変化。",
    "emitter":     "エミッター — ①パーティクルエミッター：パーティクル（粒子）の発生源。位置、速度、色などの初期値を決定する。\n②オーディオエミッター：3D空間内の音源。位置と方向を持ち、リスナーとの距離で音量が変化する。",
    "listener":    "リスナー — ①オーディオリスナー：3D空間内の「耳」の役割。通常はカメラ位置に配置する。\n②イベントリスナー：特定のイベントが発生した時に呼ばれる関数（＝コールバック）。",
    "camera":      "カメラ — 3D/2D空間における視点。\n3Dカメラ：位置、方向、視野角（FOV）、近/遠クリッピング面を持つ。ビュー行列と射影行列を生成する。\n2Dカメラ：スクロールとズームを制御する。",
    "light":       "ライト（光源） — 3Dシーンを照らす光。\nDirectional（太陽光・平行光）、Point（点光源）、Spot（スポットライト）の3種が基本。\n影の生成（シャドウマップ）やPBRライティングの入力となる。",
    "shadow":      "シャドウ（影） — ライトが遮られてできる暗い領域。\nシャドウマップ：ライト視点の深度バッファで影を判定する。\nCSM（カスケード）で広範囲に対応し、コンタクトシャドウで小さな影を補強する。",
    "normal":      "法線 — ①法線ベクトル：面や頂点の向きを表す長さ1のベクトル。ライティング計算の基本。\n②法線マップ：テクスチャに法線情報を格納し、低ポリゴンモデルに凹凸感を出す技術。",
    "UV":          "UV座標 — テクスチャのどの位置を頂点に貼るかを指定する2D座標。\nU（横）とV（縦）で0〜1の範囲。UVマッピングでテクスチャを3Dモデルに正しく貼り付ける。",
    "tangent":     "タンジェント — 接線ベクトル。法線マップの計算に必要なTBN空間（接線・従法線・法線）の一要素。\nテクスチャのU方向に沿ったベクトルで、法線マップの向きをワールド空間に変換する。",
    "albedo":      "アルベド — 物体表面の基本色（反射色）。\nPBRレンダリングでは金属の反射色や非金属のディフューズ色を表す。\nテクスチャマップとして提供されることが多い。",
    "roughness":   "ラフネス（粗さ） — PBRパラメータ。表面のマイクロファセットの粗さを0（鏡面）〜1（完全に拡散）で指定する。\n低い値で光がシャープに反射し、高い値でぼやけた反射になる。",
    "metallic":    "メタリック（金属度） — PBRパラメータ。0が非金属（プラスチック、木など）、1が金属。\n金属は入射光のほとんどを鏡面反射し、ディフューズ成分がほぼゼロになる。",
    "ambient":     "アンビエント（環境光） — 直接照明が届かない部分にも最低限の明るさを与える間接光の近似。\nSSAOで環境遮蔽を加えることでリアリティを向上させる。",
    "specular":    "スペキュラ（鏡面反射） — 光源の方向と視線の反射ベクトルが一致する部分に現れる明るいハイライト。\nラフネスが低いほどシャープなスペキュラ反射が出る。",
    "diffuse":     "ディフューズ（拡散反射） — 光が表面で全方向に均等に散乱する反射成分。\n物体の基本的な色を決める。ランバート反射モデルが最も単純な計算方法。",
    "frustum":     "視錐台 — カメラの視野を表す四角錐台の3D領域。\n近平面と遠平面で切り取られた範囲。この中にあるオブジェクトだけが描画される。\n視錐台カリングで範囲外のオブジェクトを除外する。",
    "octree":      "オクトリー（八分木） — 3D空間を再帰的に8つの子ノードに分割するツリー構造。\n空間検索（近傍オブジェクトの探索）や衝突判定の高速化に使う。",
    "quadtree":    "クアッドツリー（四分木） — 2D空間を再帰的に4つの子ノードに分割するツリー構造。\n2Dゲームの衝突判定やタイルマップの空間検索を高速化する。",
    "GJK":         "Gilbert-Johnson-Keerthi — 凸形状同士の衝突判定アルゴリズム。\nミンコフスキー差の原点包含判定で衝突を検出する。EPAと組み合わせて衝突情報を取得。",
    "EPA":         "Expanding Polytope Algorithm — GJKの結果を利用して衝突の深さと方向（分離ベクトル）を求めるアルゴリズム。\n物理エンジンの衝突応答に必要な情報を提供する。",
    "IK":          "Inverse Kinematics（逆運動学） — 目標位置からボーンの角度を逆算する技術。\n手を特定の位置に伸ばす、足を地面に接地させるなどに使う。\nCCD-IK、FABRIK、TwoBoneIKなどのアルゴリズムがある。",
    "FK":          "Forward Kinematics（順運動学） — 親ボーンから子ボーンへ順に回転を適用して末端の位置を求める。\n通常のアニメーション再生はFKで行い、環境適応にIKを重ねる。",
    "skinning":    "スキニング — ボーンの変換に応じてメッシュの頂点を変形するプロセス。\n各頂点にボーンウェイトを割り当て、ボーンの移動に追従して頂点位置を計算する。\nGPUスキニング（頂点シェーダーで計算）が一般的。",
    "instancing":  "インスタンシング — 同じメッシュを異なる位置・回転・スケールで大量描画する最適化手法。\n1回のドローコールで数千のオブジェクトを描画でき、草や木など繰り返し要素に効果的。",
    "deferred":    "ディファードレンダリング — ①ジオメトリパスでG-Buffer（法線、アルベド、深度等）に情報を書き込む。\n②ライティングパスでG-Bufferからピクセルごとにライティングを計算する。\n多数のライトを効率的に扱えるが、半透明の扱いが困難。",
    "forward":     "フォワードレンダリング — 各オブジェクトの描画時にライティングも同時に計算する方式。\nディファードより単純で半透明を扱いやすいが、ライト数に比例して負荷が増える。",
    "compute":     "コンピュートシェーダー — 描画パイプラインとは独立したGPU汎用計算。\nパーティクル、物理、画像処理、カリングなどに使う。スレッドグループ単位でDispatchする。",
    "UAV":         "Unordered Access View — シェーダーから読み書き可能なリソースビュー。\nコンピュートシェーダーの出力先やランダムアクセスバッファに使う。\nRWTexture、RWStructuredBufferなど。",
    "SRV":         "Shader Resource View — シェーダーから読み取り専用のリソースビュー。\nテクスチャやバッファをシェーダーの入力として使う時に作成する。",
    "CBV":         "Constant Buffer View — 定数バッファへのビュー。\nシェーダーに渡すパラメータ（変換行列、マテリアル情報、ライト情報等）を格納する。",
    "PSO":         "Pipeline State Object — 描画パイプラインの全状態をまとめたオブジェクト。\nシェーダー、ブレンド、ラスタライズ、深度テストなどの設定を含む。\nDX12では事前作成してSetPipelineStateで切り替える。",
    "HLSL":        "High Level Shading Language — DirectXのシェーダー記述言語。\n頂点シェーダー、ピクセルシェーダー、コンピュートシェーダーをC風の構文で書く。\n.hlslファイルに記述し、コンパイルしてGPUで実行する。",
    "GPU":         "Graphics Processing Unit — 大量の並列計算に特化したプロセッサ。\n描画処理だけでなく、コンピュートシェーダーで汎用計算（GPGPU）にも使う。\n数千のコアで同時にピクセルや頂点を処理する。",
    "CPU":         "Central Processing Unit — プログラムの逐次実行を担当するプロセッサ。\nゲームロジック、AI、物理の準備処理などを行い、GPUに描画命令を送る。\nマルチスレッドで並列化するがGPUほどの並列度はない。",
    "VRAM":        "Video RAM — GPU専用のメモリ。テクスチャ、バッファ、レンダーターゲットを格納する。\nCPU側のRAMとは別で、転送にはアップロードヒープを経由する。\n容量が限られるため、ストリーミングやLODで効率的に使う。",
    "DRS":         "Dynamic Resolution Scaling — 負荷に応じて描画解像度を動的に変更する技術。\nフレームレートが下がったら解像度を下げ、余裕があれば上げる。TAA等で品質を補完する。",
    "VRS":         "Variable Rate Shading — 画面の領域ごとにシェーディング精度を変える技術。\n視線の中心は高精度、周辺は低精度にして描画負荷を下げる。",
    "RTGI":        "Ray Traced Global Illumination — レイトレーシングを使ったグローバルイルミネーション。\n光の間接反射を物理的に正確に計算し、リアルな照明を実現する。DXRハードウェアが必要。",
    "DXR":         "DirectX Raytracing — DirectX 12のレイトレーシングAPI。\nAcceleration Structure（BVH）を使ってGPUでリアルタイムレイトレーシングを行う。",
    "NAT":         "Network Address Translation — プライベートIPアドレスをグローバルIPに変換するネットワーク技術。\nP2P通信ではNAT越え（ホールパンチング）が必要になる。",
    "TCP":         "Transmission Control Protocol — 信頼性のある順序付き通信プロトコル。\nデータの到達保証と順序保証があるが、遅延が大きい。チャットやファイル転送に適する。",
    "UDP":         "User Datagram Protocol — 低遅延だが到達保証のない通信プロトコル。\nリアルタイムゲームの位置同期に適する。信頼性が必要な場合は自前で再送制御を実装する。",
    "WebSocket":   "WebSocket — HTTPを拡張した全二重通信プロトコル。\nサーバーからクライアントへのプッシュ通知やリアルタイムデータ配信に使う。",
    "HTTP":        "HyperText Transfer Protocol — Web通信の基盤プロトコル。\nリクエスト/レスポンス型。REST APIやアセットダウンロードに使う。",
    "latency":     "レイテンシ（遅延） — データが送信元から送信先に届くまでの時間。\nネットワークでは ping で計測。ゲームでは入力から画面反映までの遅延も含む。\n低レイテンシが快適なプレイ体験の鍵。",
    // ── OOP & Design Patterns ──
    "継承":          "inheritance — 既存のクラスの機能を引き継いで新しいクラスを作る仕組み。\nclass Child : public Parent の形式。親の機能を再利用・拡張できる。",
    "カプセル化":     "encapsulation — データと操作をクラスにまとめ、外部からの不正なアクセスを防ぐ。\nprivate/protectedで実装を隠蔽し、publicメソッドだけを公開する。",
    "インターフェース":"interface — 実装を持たない純粋仮想関数だけのクラス（C++では抽象クラスで実現）。\n「何ができるか」の契約を定義し、具体的な「どうやるか」は派生クラスに任せる。",
    "委譲":          "delegation — 処理を別のオブジェクトに任せるパターン。\n継承の代わりにコンポジション（メンバとして持つ）で機能を再利用する。",
    "ファクトリ":     "factory — オブジェクトの生成を専用の関数やクラスに任せるパターン。\n生成ロジックを隠蔽し、柔軟な生成（型の切り替え等）を可能にする。",
    "オブザーバー":   "observer — あるオブジェクトの状態変化を、他のオブジェクトに自動通知するパターン。\nイベントシステムやコールバックの基盤。購読者（Observer）と発行者（Subject）からなる。",
    "ビルダー":      "builder — 複雑なオブジェクトを段階的に構築するパターン。\nメソッドチェーンで設定を積み重ね、最後にBuild()で完成させる。\nGXLibではRootSignatureBuilderやPipelineStateBuilderなど。",
    "コマンド":      "command — 操作をオブジェクトとしてカプセル化するパターン。\n操作の実行・取消・やり直しを統一的に扱える。Undo/Redoの基盤。\nGPUコマンドリストもこの考え方に近い。",
    // ── C++ Concepts (Japanese) ──
    "コンストラクタ": "constructor — オブジェクト生成時に自動で呼ばれる初期化関数。\nクラス名と同じ名前で、戻り値はない。メンバ変数の初期化を行う。",
    "デストラクタ":   "destructor — オブジェクト破棄時に自動で呼ばれる後処理関数。\n~クラス名()の形式。メモリやリソースの解放を行う。\nポリモーフィズムを使う場合はvirtualを付ける。",
    "コールバック":   "callback — あるイベントの発生時に呼び出される関数。\nstd::functionや関数ポインタで登録し、後から呼び出される。\nボタンクリックやタイマー完了時などに使う。",
    "シングルトン":   "singleton — インスタンスが1つだけのデザインパターン。\nInstance()等のstaticメソッドでアクセスする。グローバル状態の管理に使う。",
    "イテレータ":     "iterator — コンテナの要素を1つずつ走査するオブジェクト。\nbegin()〜end()のrangeベースforループで使う。ポインタに似た操作。",
    "ポリモーフィズム":"polymorphism（多態性） — 同じ関数名で型ごとに異なる振る舞いをする仕組み。\nvirtual関数とポインタ/参照を組み合わせて実現する。",
    "テンプレート":   "template — 型に依存しない汎用コードを書く仕組み。\ntemplate<typename T>で型パラメータを宣言する。\nstd::vectorやstd::functionなどSTLの基盤技術。",
    "オーバーライド": "override — 親クラスの仮想関数を子クラスで再定義すること。\noverrideキーワードを付けるとシグネチャの不一致をコンパイル時に検出できる。",
    "オーバーロード": "overload — 同名の関数を引数の型や数を変えて複数定義すること。\n呼び出し時に引数に最も合う関数がコンパイラによって自動選択される。",
    "スマートポインタ":"smart pointer — メモリを自動管理するポインタ。\nunique_ptr（単独所有・最速）やshared_ptr（共有所有・参照カウント）がある。\nスコープを抜けると自動でdeleteされる。生のポインタより安全。",
    "ラムダ":        "lambda（ラムダ式） — 名前のない関数を書く構文。[キャプチャ](引数){ 本体 } の形式。\nコールバックやSTLアルゴリズムに渡す短い処理を書くのに便利。\n[=]で値キャプチャ、[&]で参照キャプチャ。",
    "ムーブ":        "move semantics — オブジェクトのリソースを「移動」させ、コピーを避ける仕組み。\nstd::move()でムーブを明示する。大きなオブジェクトの所有権移動に高速。\nコピーでは「複製」、ムーブでは「引っ越し」のイメージ。",
    "参照":          "reference — 既存の変数に別名を付ける仕組み。int& ref = x; のように&で宣言する。\nポインタと違いnullにならず、宣言時に必ず初期化が必要。\nconst参照（const T&）は一時オブジェクトも受け取れる。",
    "ポインタ":      "pointer — メモリ上のアドレスを保持する変数。int* ptr = &x; のように*で宣言する。\n->でメンバアクセス、*で間接参照。nullptrを取りうるので注意。\n現代C++ではスマートポインタの使用が推奨される。",
    "スコープ":      "scope — 変数や関数が有効な範囲。{}で囲まれたブロックが1つのスコープ。\nスコープを抜けるとローカル変数は自動で破棄される（RAII）。\nネームスコープ、クラススコープ、ブロックスコープなどがある。",
    "前方宣言":      "forward declaration — クラスや関数を「存在する」とだけ宣言し、定義は後回しにすること。\nヘッダの相互依存を解消し、コンパイル時間を短縮する。class MyClass; の形式。",
    "列挙型":        "enum — 名前付き定数の集合を定義する型。\nenum class を使うとスコープ付き（型安全）で使える。\n状態やフラグの表現に適している。",
    "基底クラス":     "base class（親クラス） — 他のクラスに継承される元のクラス。\n共通の機能をまとめ、派生クラスで拡張する。virtual関数で拡張ポイントを提供する。",
    "派生クラス":     "derived class（子クラス） — 基底クラスを継承して機能を拡張したクラス。\n親の機能を引き継ぎつつ、新しい機能を追加したりvirtual関数を上書きできる。",
    "抽象クラス":     "abstract class — 純粋仮想関数(= 0)を持つクラス。\n直接インスタンス化できず、派生クラスで実装を提供する。\nインターフェースの定義に使う。",
    "インスタンス":   "instance — クラスから実際に作られたオブジェクト。\nMyClass obj; や new MyClass() で生成する。\n同じクラスから複数の異なるインスタンスを作れる。\nGPUインスタンシングでは同一メッシュの大量複製を指す。",
    "アロケータ":     "allocator — メモリの確保・解放を管理するオブジェクト。\nカスタムアロケータでメモリ戦略（プール、スタック等）を制御できる。\nゲームエンジンでは高速化のために独自アロケータを使うことが多い。",
    "ミューテックス": "mutex（相互排他） — 複数スレッドが同時にデータにアクセスするのを防ぐ。\nlock()で排他開始、unlock()で解除。std::lock_guardで自動管理が安全。",
    "スレッド":      "thread — プログラム内で並行して実行される処理の単位。\nstd::threadで生成する。複数スレッドで同じデータにアクセスする場合はミューテックスで排他制御が必要。",
    "デッドロック":   "deadlock — 2つ以上のスレッドが互いのロック解除を待ち合い、永久に停止する状態。\nロック順序を統一するか、std::lock()で同時ロックして回避する。",
    "非同期":        "async — 処理の完了を待たずに次の処理に進む実行方式。\nstd::asyncやコールバックで実現する。UIのフリーズ防止やI/O待ちの効率化に使う。",
    "同期":          "sync — ①処理の完了を待ってから次に進む実行方式。\n②マルチスレッドでは排他制御によるデータの一貫性保証。\n③GPU同期：フェンスやバリアでCPU/GPU間の実行順序を制御。",
    // ── Memory & Data Structures ──
    "ヒープ":        "heap — ①メモリヒープ：動的にメモリを確保する領域。new/deleteで管理する。\n②データ構造のヒープ：優先度キューの基盤となる完全二分木。\n③GPUヒープ：ディスクリプタヒープやアップロードヒープなどGPUリソースの管理単位。",
    "スタック":      "stack — ①メモリスタック：関数呼び出しごとに自動確保/解放されるメモリ領域。高速だがサイズ制限あり。\n②データ構造のスタック：LIFO（後入先出）。Undo操作やパーサーに使う。\n③レイヤースタック：描画の重ね順序管理。",
    "メモリリーク":   "memory leak — 確保したメモリを解放し忘れ、使えないメモリが増え続けるバグ。\nスマートポインタやRAIIで防ぐ。長時間動作するプログラムでは致命的。",
    "メモリプール":   "memory pool — 事前に大きなメモリブロックを確保し、小さな単位で切り出して使う仕組み。\nnew/deleteを繰り返すよりも高速で、フラグメンテーションも防げる。",
    "キュー":        "queue（待ち行列） — 先に入れたものが先に出る（FIFO）データ構造。\nタスク管理やイベント処理でよく使う。コマンドキューはGPUへの命令待ち行列。",
    "ハッシュ":      "hash — データを固定長の値に変換する関数/その結果。\nハッシュマップ（unordered_map）で高速な検索に使う。\nファイルの整合性チェック（MD5, SHA等）にも使う。",
    "配列":          "array — 同じ型の要素を連続メモリに並べたデータ構造。\nインデックスでO(1)アクセス。std::arrayは固定長、std::vectorは可変長。\nキャッシュ効率が良く、ゲームエンジンでは最も重要なデータ構造。",
    "マップ":        "map — キーと値のペアを格納する連想コンテナ。\nstd::mapはソート済み（赤黒木）、std::unordered_mapはハッシュベース。\nキーから値をO(log n)またはO(1)で検索できる。",
    // ── Graphics & Rendering ──
    "バッファ":      "buffer — データを一時的に格納するメモリ領域。\n頂点バッファ、インデックスバッファ、定数バッファなどGPU側のメモリとして使う。\nダブルバッファリングでフレーム間のデータ競合を防ぐ。",
    "シェーダー":     "shader — GPUで実行される小さなプログラム。\n頂点シェーダー（形状変換）、ピクセルシェーダー（色計算）、コンピュートシェーダー（汎用計算）がある。\nHLSLやGLSLで記述する。",
    "レンダーターゲット":"render target — GPUの描画結果を書き込むテクスチャ。\n画面に直接描くか、後処理（ポストエフェクト）用の中間バッファとして使う。\nMRT（Multiple Render Targets）で同時に複数に書き込める。",
    "コマンドリスト": "command list — GPUに送る描画命令をまとめたリスト。\nDraw、Dispatch等の命令を記録し、キューに送信して一括実行する。\nDX12ではマルチスレッドで並列記録できる。",
    "パイプライン":   "pipeline — ①描画パイプライン：頂点処理→ラスタライズ→ピクセル処理の描画の流れ。PSO（Pipeline State Object）で設定する。\n②ポストエフェクトパイプライン：複数のポストエフェクトを連鎖的に適用する仕組み。",
    "メッシュ":      "mesh — 3Dモデルの形状データ。\n頂点（位置・法線・UV）とインデックス（三角形の頂点番号）からなる。\nプリミティブ（三角形）の集合体。サブメッシュで部位ごとにマテリアルを分ける。",
    "マテリアル":     "material — 3Dオブジェクトの表面の見た目を定義するデータ。\nテクスチャ、色、金属度、粗さなどを含む。シェーダーと組み合わせて使う。\nPBR（物理ベース）、Toon（トゥーン）、Unlit（ライティングなし）などの種類がある。",
    "テクスチャ":     "texture — 3Dオブジェクトの表面に貼る画像データ。\nGPUメモリに置かれ、シェーダーからサンプリングされる。\nミップマップで距離に応じた解像度を使い分ける。",
    "ミップマップ":   "mipmap — テクスチャの縮小版を段階的に用意したもの。\n遠くのオブジェクトには小さいミップを使い、描画品質とパフォーマンスを両立する。",
    "デプスバッファ": "depth buffer（深度バッファ） — 各ピクセルの奥行き情報を格納するバッファ。\n手前のオブジェクトで奥のオブジェクトを隠す（深度テスト）に使う。",
    "ブレンド":      "blend — 色の合成方法。半透明描画やパーティクルの加算合成などに使う。\nソース色とデスティネーション色をどう混ぜるかを指定する。",
    "サンプラー":     "sampler — テクスチャからピクセルの色を読み取る方法を定義するオブジェクト。\nフィルタリング（線形/最近傍）やアドレスモード（繰り返し/クランプ）を設定する。",
    "ルートシグネチャ":"root signature — シェーダーがアクセスするリソース（テクスチャ、バッファ等）のレイアウト定義。\nDX12特有の概念で、パイプラインとリソースの橋渡し。",
    "スワップチェーン":"swap chain — 画面表示用のバッファを複数持ち、交互に切り替える仕組み。\nダブルバッファリング（2枚）やトリプルバッファリング（3枚）でちらつきを防ぐ。",
    "フレームバッファ":"frame buffer — 画面に表示される最終的な画像を格納するバッファ。\nカラー、深度、ステンシルの各バッファを含む。",
    "ステンシル":     "stencil — ピクセルごとのマスク値を格納するバッファ。\n特定の領域のみ描画/非描画を制御する。アウトライン、ポータル、マスク処理に使う。",
    "ラスタライズ":   "rasterize — 3Dの三角形をピクセル（2Dの画素）に変換する処理。\nGPUパイプラインで頂点処理の後に自動実行される。",
    "カリング":      "culling — 見えないオブジェクトを描画対象から除外する最適化。\n視錐台カリング、バックフェースカリング、オクルージョンカリングなど。",
    "フェンス":      "fence — CPUとGPUの同期を取るためのオブジェクト。\nGPUの処理完了をCPU側で待機する時に使う。DX12の必須同期プリミティブ。",
    "スプライト":     "sprite — 2Dの画像オブジェクト。キャラクター、アイテム、背景パーツなどに使う。\nスプライトシートから一部を切り出して表示する。SpriteBatchで効率的にまとめて描画する。",
    "タイルマップ":   "tilemap — 小さなタイル画像を格子状に並べて地形やマップを構成する手法。\n2Dゲームの背景に広く使われる。TMJ/TMXなどのエディタ形式で定義する。",
    "パーティクル":   "particle — 大量の小さな粒子で煙、火、爆発、雪などのエフェクトを表現する技術。\n各粒子は位置・速度・寿命・色を持ち、毎フレーム更新する。\nGPUパーティクルでは数百万粒子をリアルタイム処理できる。",
    "トレイル":      "trail — オブジェクトの移動軌跡を描画するエフェクト。\n剣の軌跡、弾の尾、移動の残像などに使う。メッシュを動的に生成する。",
    "デカール":      "decal — 既存のジオメトリ表面に追加で投影されるテクスチャ。\n弾痕、血痕、落書きなど環境に動的に貼り付けるエフェクト。プロジェクションで投影する。",
    // ── Post Effects & Techniques ──
    "SSAO":         "Screen Space Ambient Occlusion — 画面空間で環境遮蔽（影の溜まり）を近似するポストエフェクト。\n物体の凹んだ部分に柔らかい影を付けて立体感を増す。",
    "SSR":          "Screen Space Reflections — 画面空間で反射を計算するポストエフェクト。\nレイマーチングで画面内の他のピクセルを反射として利用する。",
    "TAA":          "Temporal Anti-Aliasing — 時間方向の情報を使ったアンチエイリアス。\n前フレームと現フレームをブレンドしてジャギーを軽減する。",
    "FXAA":         "Fast Approximate Anti-Aliasing — 高速な画像ベースのアンチエイリアス。\n最終画像のエッジを検出してぼかすことでジャギーを軽減する。軽量だがやや不鮮明。",
    "PBR":          "Physically Based Rendering（物理ベースレンダリング） — 現実の光の振る舞いに基づいた描画手法。\n金属度、粗さなどの物理パラメータでマテリアルを定義する。",
    "IBL":          "Image Based Lighting — 環境マップ画像を光源として使うライティング手法。\n周囲の環境を反映したリアルな照明を実現する。",
    "HDR":          "High Dynamic Range — 通常の0〜1の範囲を超えた明るさを扱う技術。\n太陽や光源の輝きをリアルに表現し、トーンマッピングで最終表示に変換する。",
    "LOD":          "Level of Detail — 距離に応じてモデルの詳細度を切り替える最適化。\n遠いオブジェクトはポリゴン数を減らして描画負荷を下げる。",
    "CSM":          "Cascaded Shadow Maps — 視点からの距離に応じて複数のシャドウマップを使い分ける手法。\n近くは高精度、遠くは低精度のシャドウマップで効率的に影を描画する。",
    "DOF":          "Depth of Field（被写界深度） — カメラのピント範囲外をぼかすエフェクト。\n焦点の合った部分は鮮明に、手前や奥はぼやけて表示される。",
    "トーンマッピング":"tone mapping — HDR画像をSDR（通常の画面表示範囲）に変換する処理。\nACES、Reinhardなどの演算子があり、白飛びを防ぎつつ明暗の表現を維持する。",
    "ブルーム":      "bloom — 明るい部分が周囲ににじみ出す光の効果。\n高輝度ピクセルを抽出→ぼかし→元画像に加算合成する。太陽や光源の眩しさを表現。",
    "モーションブラー":"motion blur — 移動するオブジェクトや高速なカメラ回転にぶれを加えるエフェクト。\n速度バッファを使い、移動方向にピクセルをぼかして残像を表現する。",
    "ビネット":      "vignette — 画面の四隅を暗くするポストエフェクト。\nカメラレンズの光量落ちを再現し、視線を画面中央に誘導する効果がある。",
    "色収差":        "chromatic aberration — レンズの色ズレを再現するポストエフェクト。\nR/G/Bチャンネルを微妙にずらして描画し、レンズの質感を出す。",
    "フィルムグレイン":"film grain — フィルム撮影のようなノイズを画面に加えるエフェクト。\n映画的な雰囲気やレトロ感を演出する。",
    // ── Math ──
    "ベクトル":      "vector — 大きさと方向を持つ量。位置、速度、力などの表現に使う。\nVector2（2D）、Vector3（3D）、Vector4（同次座標）がある。\nSTLのstd::vectorはこれとは別の動的配列。",
    "行列":          "matrix — 変換（移動、回転、拡縮）を表現する数学的な構造。\n4x4行列で3Dの座標変換を行う。行列の積で変換を合成できる。",
    "クォータニオン": "quaternion — 3D回転を表現する4次元の数。\nオイラー角（XYZ回転）より安全で、ジンバルロックが起きない。\n球面線形補間（Slerp）で滑らかな回転補間ができる。",
    "正規化":        "normalize — ベクトルの長さを1にする操作。方向だけを取り出す。\n法線ベクトルやライティング計算で頻繁に使う。",
    "内積":          "dot product — 2つのベクトルの類似度を求める演算。\n結果が正なら同じ方向、0なら直交、負なら逆方向。ライティングの基本。",
    "外積":          "cross product — 2つのベクトルに直交するベクトルを求める演算（3Dのみ）。\n面の法線計算や回転軸の算出に使う。",
    "線形補間":      "linear interpolation（lerp） — 2つの値の間を割合（0〜1）で滑らかにつなぐ。\nlerp(a, b, t) = a + (b - a) * t。アニメーションや色の遷移に使う。",
    "スプライン":     "spline — 滑らかな曲線を複数の制御点から生成する数学的手法。\nカメラの移動経路やアニメーションカーブに使う。\nCatmull-Rom、Bezier、B-スプラインなどの種類がある。",
    "イージング":     "easing — 動きの加速・減速を制御する関数。\nEaseIn（最初はゆっくり→加速）、EaseOut（最初は速く→減速）、EaseInOut（両端がゆっくり）。\nUIアニメーションやTweenで自然な動きを作る。",
    "SIMD":         "Single Instruction, Multiple Data — 1命令で複数のデータを同時に処理するCPU命令セット。\nSSE/AVXでfloat×4を同時計算。ベクトル/行列演算の高速化に使う。\nDirectXMathはSIMDを内部的に活用している。",
    "AABB":         "Axis-Aligned Bounding Box — 座標軸に平行な矩形の当たり判定領域。\n回転しないので計算が速い。大まかな衝突判定の第一段階で使う。",
    "OBB":          "Oriented Bounding Box — 任意の向きを持つ矩形の当たり判定領域。\nAABBより形状にフィットするが計算コストが高い。分離軸定理（SAT）で判定する。",
    "BVH":          "Bounding Volume Hierarchy — 当たり判定用のツリー構造。\nオブジェクトをバウンディングボックスの階層で管理し、効率的に衝突判定する。",
    // ── Physics ──
    "剛体":          "rigid body — 物理シミュレーションで変形しない物体。\n質量、速度、角速度を持ち、重力や衝突の力で動く。Dynamic（動的）、Static（静的）、Kinematic（運動学的）の3種。",
    "コライダー":     "collider — 物体の当たり判定の形状。\n球、箱、カプセル、メッシュなどの形状がある。実際の見た目とは別に設定できる。\nトリガーモードでは物理応答なしで衝突を検出するだけ。",
    "レイキャスト":   "raycast — レイ（光線）を飛ばして最初にぶつかる物体を検出する技術。\n視線判定、弾道計算、地面検出などに使う。",
    "ジョイント":     "joint（関節/拘束） — 2つの剛体を接続して動きを制限する物理拘束。\nヒンジ（蝶番）、ボールソケット（球関節）、スプリング（ばね）などの種類がある。\nドアの開閉やラグドールに使う。",
    "トリガー":      "trigger — 物理的な衝突応答を持たず、オブジェクトの進入/退出だけを検出するコライダー。\nアイテム取得範囲、ダメージゾーン、イベント発火エリアなどに使う。",
    "ラグドール":     "ragdoll — キャラクターの死亡時などにボーンを剛体+ジョイントで制御し、物理的に自然な倒れ方をさせる技術。\n各ボーンに剛体を割り当て、関節拘束で接続する。",
    // ── Audio ──
    "サンプルレート": "sample rate — 1秒間に音声データを何回サンプリングするか（例: 44100Hz, 48000Hz）。\n高いほど高音質だがデータ量が増える。",
    "DSP":          "Digital Signal Processing — デジタル信号処理。音声にリバーブ、EQ、コンプレッサーなどのエフェクトを適用する技術。",
    "ミキサー":      "mixer — 複数の音声信号を合成（ミックス）する仕組み。\n各音源の音量、パン、エフェクトを個別に制御してから1つの出力にまとめる。\nバス（Bus）で階層的にグループ化する。",
    "リバーブ":      "reverb — 空間の残響を再現するオーディオエフェクト。\n音が壁や天井で反射して徐々に減衰する効果。部屋の大きさや素材で特性が変わる。",
    "パン":          "pan（パンニング） — 音声を左右のスピーカーに振り分ける操作。\n0.0で中央、-1.0で左、1.0で右。3Dサウンドでは自動計算される。",
    "3Dサウンド":    "3D sound（空間音響） — 3D空間内の音源の位置と聞き手の位置関係に応じて音量・パンを自動計算する技術。\n距離減衰、ドップラー効果、遮蔽を再現する。",
    // ── Game Engine Concepts ──
    "フレームレート": "frame rate — 1秒間に画面を何回更新するかの指標（FPS）。\n60FPSなら1/60秒（約16.7ms）ごとに1フレーム。滑らかな動きの基準。",
    "デルタタイム":   "delta time — 前フレームから現フレームまでの経過時間。\n移動量 = 速度 × deltaTime のように使い、フレームレートに依存しない動きを実現する。",
    "エンティティ":   "entity — ①ゲームワールド内の個々のオブジェクト（敵、アイテム、地形など）。\n②ECSでは単なるID（整数）。データは全てコンポーネントが持つ。\n③シーン内のノード：コンポーネントを付けて機能を構成する。GXLibではEntity IDとComponentの組合せ。",
    "コンポーネント": "component — ①エンティティに付加する機能/データの単位（Transform、Renderer、Collider等）。\n②ECSではデータのみを持つ構造体。ロジックはSystemが担当する。\n③GUIコンポーネント：ボタン、スライダー等のUI部品。\n組み合わせてオブジェクトの機能を構成する設計パターン。",
    "システム":      "system — ①ECSでコンポーネントのデータを処理するロジック。MovementSystem、RenderSystemなど。\n②ゲームエンジンの各サブシステム（描画システム、物理システム、入力システム等）。\n③OSのシステム（ファイルシステム等）。",
    "シーン":        "scene — ①ゲームの場面/ステージ。エンティティ、カメラ、ライトなどを含む。タイトル→ゲーム→リザルトを遷移する。\n②3Dシーン：3D空間のオブジェクト群。SceneRendererで描画する。\n③GXLibではCore/Scene/Scene.hで定義。描画はGraphics/3D/SceneRenderer.hが担当。",
    "ワールド":      "world — ①ECSのWorld：全てのエンティティとコンポーネントを管理するコンテナ。\n②ゲームワールド：プレイヤーが体験するゲーム空間全体。\n③ワールド座標系：グローバルな3D座標空間。ローカル座標の対義語。",
    "プレハブ":      "prefab（プレファブ） — エンティティの設計図（テンプレート）。\nコンポーネントの構成と初期値を保存し、何度でもインスタンス化できる。\n敵、アイテム、弾丸など同じ構成のオブジェクトを量産する時に使う。",
    "ECS":          "Entity Component System — データ指向の設計パターン。\nEntity（ID）にComponent（データ）を付け、System（ロジック）で処理する。\nキャッシュ効率が良く、大量のオブジェクトを高速に処理できる。\nGXLibではGXLib_ECSモジュールで実装。",
    "ゲームループ":   "game loop — ゲームの基本的な繰り返し処理。\n「入力取得→状態更新→描画」を毎フレーム繰り返す。",
    "アーキタイプ":   "archetype — ECSでコンポーネントの組み合わせが同じエンティティをまとめたグループ。\n同じアーキタイプのデータはメモリ上に連続配置され、キャッシュ効率が高い。",
    "クエリ":        "query — ①ECSのクエリ：特定のコンポーネントを持つエンティティを検索する仕組み。\n②データベースクエリ：データの検索・取得命令。\n③レイキャストクエリ：物理世界への問い合わせ。",
    "ナビメッシュ":   "navigation mesh（NavMesh） — AIキャラクターが歩行可能な領域を定義する三角形メッシュ。\n経路探索（A*等）の基盤。障害物を避けながら目的地への最短経路を計算する。",
    "ビヘイビアツリー":"behavior tree — AIの意思決定を木構造で表現する手法。\nSequence（順次実行）、Selector（選択実行）、条件/アクションノードで構成する。\n複雑なAIの振る舞いを視覚的に設計できる。",
    "ステートマシン": "state machine（有限状態機械） — 有限個の状態と遷移条件で動作を制御する設計パターン。\nアニメーション（Idle→Walk→Run）、AI（Patrol→Chase→Attack）、ゲームフローの管理に使う。",
    "ウィジェット":   "widget — GUIの基本部品。ボタン、テキスト、スライダー、パネルなど。\n親子関係を持ち、レイアウトとスタイルで見た目を制御する。\nユーザーの入力（クリック、ドラッグ等）に応答する。",
    "スタイルシート": "style sheet — GUIウィジェットの見た目（色、フォント、マージン等）を定義するデータ。\nCSSに似た仕組みでウィジェットに外観を適用する。",
    "シリアライズ":   "serialize — オブジェクトの状態をバイト列やテキスト（JSON、XML、バイナリ）に変換すること。\nファイル保存、ネットワーク送信、シーンの永続化に使う。逆変換はデシリアライズ。",
    "アセット":      "asset — ゲームで使うデータリソースの総称。\nテクスチャ、3Dモデル、サウンド、シェーダー、スクリプトなど。\nAssetDatabaseで管理し、非同期ロードで読み込む。",
    "ギズモ":        "gizmo — エディタで3Dオブジェクトの移動・回転・拡縮を操作するための視覚的なハンドル。\n赤(X)・緑(Y)・青(Z)の矢印やリングで軸ごとに操作できる。",
    // ── Very fine-grained terms (basics & compound words) ──
    "型":            "type — データの種類を定義する概念。int, float, bool, classなど。\n型が決まることで変数のサイズ、演算、使い方がコンパイラに伝わる。\nC++は静的型付け言語で、コンパイル時に型チェックが行われる。",
    "描画":          "rendering/draw — GPUを使って画面に映像を出力する処理。\n2D描画（スプライト、テキスト）と3D描画（メッシュ、ライティング）がある。\nドローコール1回でGPUに1つの描画命令を送る。",
    "変数":          "variable — データを格納する名前付きのメモリ領域。\n型と名前で宣言し、値を代入・参照できる。ローカル変数はスコープを抜けると消える。",
    "関数":          "function — 一連の処理をまとめた再利用可能なコードの塊。\n引数（入力）を受け取り、戻り値（出力）を返す。メンバ関数はクラスに属する関数（メソッド）。",
    "メソッド":      "method — クラスに属する関数。メンバ関数とも呼ぶ。\nthisポインタを通じてオブジェクトのメンバにアクセスできる。",
    "クラス":        "class — データ（メンバ変数）と操作（メンバ関数）をまとめた設計図。\nnewやスタック上でインスタンス化して使う。継承でポリモーフィズムを実現する。\nデフォルトのアクセスはprivate（structはpublic）。",
    "メンバ":        "member — クラスや構造体に属する変数（メンバ変数/フィールド）や関数（メンバ関数/メソッド）。\nm_プレフィックスが付くことが多い。staticメンバはクラス全体で共有される。",
    "フィールド":     "field — クラスや構造体のメンバ変数。オブジェクトの状態を保持するデータ。\nGXLibではm_プレフィックスが慣例。publicフィールドは外部から直接アクセスできる。",
    "引数":          "argument/parameter — 関数に渡すデータ。\n定義側のパラメータ(int x)と呼出側のアーギュメント(func(42))がある。\n値渡し、参照渡し、ポインタ渡しの3方式。",
    "戻り値":        "return value — 関数が呼び出し元に返す値。\nreturn文で指定する。voidの関数は戻り値がない。",
    "初期化":        "initialization — 変数やオブジェクトに最初の値を設定すること。\n未初期化変数の読み取りは未定義動作。コンストラクタ初期化リストが推奨される。",
    "宣言":          "declaration — 名前と型をコンパイラに教えること。\n定義（definition）は実体のメモリを確保する。ヘッダには宣言、cppには定義を書くのが一般的。",
    "定義":          "definition — 変数や関数の実体を作ること。\n宣言は「存在を通知」、定義は「実体を作成」。ODR（One Definition Rule）で1つだけ定義できる。",
    "バインド":      "bind — ①リソースバインド：シェーダーにテクスチャやバッファを接続すること。\n②スクリプトバインド：C++の関数やクラスをLua等から呼べるようにすること。\n③キーバインド：入力キーとアクションの対応付け。\n④std::bind：関数の引数を部分的に固定する仕組み。",
    "コンテナ":      "container — データを格納・管理するデータ構造の総称。\nvector、map、set、listなどSTLのコレクション。\n用途に応じて適切なコンテナを選ぶことがパフォーマンスの鍵。",
    "ノード":        "node — ①ツリーやグラフの構成要素。親子関係を持つ。\n②シーングラフのノード：3Dオブジェクトの階層構造の1要素。\n③ビヘイビアツリーのノード：AI判断の1ステップ。\n④ノードグラフ：ビジュアルプログラミングの接続点。",
    "パス":          "path — ①ファイルパス：ファイルの場所を示す文字列。\n②レンダーパス：描画処理の1ステップ（ジオメトリパス、ライティングパスなど）。\n③経路：ナビメッシュ上のAIの移動経路。",
    "オフセット":     "offset — 基準位置からのずれ量（バイト数や座標値）。\nバッファ内の位置指定、UV座標の移動、ボーンの相対位置などに使う。",
    "ストライド":     "stride — メモリ上で次の要素までの間隔（バイト数）。\n頂点バッファのストライドは1頂点のデータサイズ。構造体のパディングに影響される。",
    "パディング":     "padding — ①メモリパディング：アライメントのために挿入される余白バイト。\nCPU/GPUはアラインされたアドレスに高速アクセスするため、構造体にパディングが入る場合がある。\n②UIパディング：ウィジェットの内側の余白。コンテンツと枠の間の隙間。",
    "プロファイル":   "profile — ①パフォーマンス計測：CPU/GPU処理時間の計測と分析。ボトルネックの特定に使う。\n②プロファイラー：計測ツール。GPUProfilerやCPUタイマーで各処理の所要時間を可視化する。",
    "ボトルネック":   "bottleneck — システム全体の性能を制限している最も遅い部分。\nCPUバウンド（CPU処理が遅い）かGPUバウンド（描画が遅い）かを特定して最適化する。",
    "フレームグラフ": "frame graph — レンダーパスの依存関係をグラフ構造で管理する仕組み。\nリソースの生存期間を自動管理し、バリアの挿入を最適化する。\nモダンなレンダリングエンジンの基盤技術。",
    // ── Compound graphics terms (must be longer to match before substrings) ──
    "メッシュレット": "meshlet — メッシュを小さな三角形グループ（32〜128三角形程度）に分割したもの。\nメッシュシェーダーと組み合わせて、GPU駆動のカリングや描画を効率化する。",
    "メッシュシェーダー":"mesh shader — DX12の新しいシェーダーステージ。頂点/ジオメトリシェーダーを置き換える。\nメッシュレット単位でGPU側でジオメトリを生成・カリングできる。タスクシェーダーと連携。",
    "コンピュートシェーダー":"compute shader — 描画パイプラインとは独立したGPU汎用計算シェーダー。\nパーティクル、物理、画像処理、カリングなどに使う。スレッドグループ単位でDispatchする。",
    "頂点シェーダー": "vertex shader — 各頂点に対して実行されるシェーダー。\n頂点の座標変換（モデル→ワールド→ビュー→プロジェクション）を行う。スキニングもここで実行。",
    "ピクセルシェーダー":"pixel shader（フラグメントシェーダー） — 各ピクセルの色を計算するシェーダー。\nライティング、テクスチャサンプリング、マテリアル計算を行う。描画負荷の大部分を占める。",
    "ジオメトリシェーダー":"geometry shader — プリミティブ（三角形等）単位で処理するシェーダー。\n新しい頂点の追加やプリミティブの変形ができるが、パフォーマンスが悪いため非推奨。\nメッシュシェーダーが後継。",
    "レイトレーシング":"ray tracing — 光線（レイ）を追跡して写実的な描画を行う技術。\n反射、屈折、影、グローバルイルミネーションをリアルに計算する。\nDXR（DirectX Raytracing）でGPUハードウェアアクセラレーションが可能。",
    "グローバルイルミネーション":"global illumination（GI） — 直接光だけでなく、物体間の光の反射（間接光）も計算する照明手法。\n写実的な明るさの表現。リアルタイムではRTGI、ライトプローブ、IRマップ等で近似する。",
    "シャドウマップ": "shadow map — ライト視点で深度バッファをレンダリングし、影を判定する技術。\nメインカメラの描画時にシャドウマップを参照して影かどうかを判定する。PCFでエッジをぼかす。",
    "スカイボックス": "skybox — 6面のテクスチャ（キューブマップ）で空や背景を表現する技術。\nカメラの位置に関係なく無限遠に見える。IBL（環境ライティング）のソースにもなる。",
    "ポストエフェクト":"post effect（ポストプロセス） — 3D描画後の最終画像に適用する画面効果の総称。\nBloom、SSAO、SSR、TAA、DOF、トーンマッピングなど。フルスクリーンの画像処理。",
    "フォワードレンダリング":"forward rendering — 各オブジェクトの描画時にライティングも同時に計算する方式。\n単純で半透明を扱いやすいが、ライト数に比例して負荷が増える。",
    "ディファードレンダリング":"deferred rendering — G-Bufferに情報を書き込み、後からライティングする方式。\n多数のライトを効率的に扱えるが、半透明の扱いが困難。",
    "テクスチャストリーミング":"texture streaming — テクスチャをミップレベル単位でオンデマンドに読み込む技術。\nVRAM使用量を削減し、必要な解像度だけをロードする。",
    "バーチャルテクスチャ":"virtual texture — 巨大なテクスチャを小さなタイル単位でオンデマンドにGPUメモリにロードする技術。\n地形やメガテクスチャで使用。実際に必要なタイルだけをメモリに配置する。",
    "インダイレクト描画":"indirect draw — CPUではなくGPU側で描画パラメータ（頂点数、インスタンス数等）を決定する描画方式。\nGPU駆動レンダリングの基盤。ExecuteIndirectで実行する。",
    "オクルージョンカリング":"occlusion culling — 他のオブジェクトに完全に隠れているオブジェクトを描画から除外する最適化。\nHi-Zバッファを使ったGPUベースの手法が主流。",
    // ── Specific algorithm/math terms ──
    "Slerp":        "Spherical Linear Interpolation（球面線形補間） — クォータニオンの回転を滑らかに補間する関数。\n2つの回転の間を最短経路で等速に補間する。Lerpより回転の品質が高い。",
    "Lerp":         "Linear Interpolation（線形補間） — 2つの値の間を直線的に補間する関数。\nlerp(a, b, t) = a + (b - a) * t。位置、色、スケールなどあらゆる補間の基本。",
    "Nlerp":        "Normalized Linear Interpolation — クォータニオンのLerp結果を正規化した補間。\nSlerpより高速だが、等速性が保証されない。短い回転では十分な品質。",
    "SDF":          "Signed Distance Field — 各点から最も近い表面までの符号付き距離を格納するデータ。\n正なら外側、負なら内側。テキスト描画（エッジが綺麗）やレイマーチング、コリジョンに使う。",
    "FFT":          "Fast Fourier Transform（高速フーリエ変換） — 信号を周波数成分に分解する高速アルゴリズム。\nオーディオ解析、海面シミュレーション、画像処理に使う。",
    // ── Additional common terms ──
    "デバッグ":      "debug — プログラムのバグ（不具合）を発見・修正すること。\nブレークポイント、ログ出力、ステップ実行でプログラムの動作を調査する。\nRelease（最適化あり）とDebug（デバッグ情報あり）ビルドを使い分ける。",
    "最適化":        "optimization — プログラムの実行速度やメモリ使用量を改善すること。\nプロファイリングでボトルネックを特定してから行う。早すぎる最適化は避ける。",
    "フレーム":      "frame — ゲームループの1周期で生成される1枚の画面。\n60FPSなら毎秒60フレーム。各フレームで入力→更新→描画を行う。",
    "ピクセル":      "pixel（画素） — 画面を構成する最小単位の点。\n解像度1920×1080なら約200万ピクセル。各ピクセルの色をピクセルシェーダーが計算する。",
    "座標系":        "coordinate system — 空間上の位置を数値で表すための基準。\nワールド座標（グローバル）、ローカル座標（オブジェクト基準）、スクリーン座標（画面上）がある。\nDX12は左手座標系を使用。",
    "ワールド座標":   "world space — ゲーム世界全体で共有されるグローバルな座標系。\n全てのオブジェクトの位置はワールド座標で表現される。ローカル→ワールド変換にモデル行列を使う。",
    "ローカル座標":   "local space（オブジェクト空間） — オブジェクト自身を原点とした座標系。\nモデルのメッシュデータはローカル座標で定義される。親のトランスフォームで階層的に変換。",
    "スクリーン座標": "screen space（画面空間） — 画面上のピクセル位置で表す座標系。\n左上が(0,0)、右下が(幅,高さ)。3D座標→スクリーン座標の変換を射影と呼ぶ。",
    "ビュー行列":     "view matrix — カメラの位置と方向に基づいてワールド座標をカメラ座標に変換する行列。\n「カメラを動かす」のではなく「世界をカメラの前に持ってくる」変換。",
    "射影行列":      "projection matrix — 3D座標を2D画面座標に変換する行列。\n透視投影（遠近感あり）と正射影（遠近感なし）の2種類。視野角(FOV)で画角を制御する。",
    "プリミティブ":   "primitive — 描画の最小単位となる図形。\n三角形（Triangle）が最も一般的。ライン、ポイントもある。3つの頂点で1つの三角形プリミティブ。",
    "ポリゴン":      "polygon — 頂点を結んで作る多角形。3Dモデルは三角形ポリゴンの集合体。\nポリゴン数が多いほど滑らかだが描画負荷が増える。LODで距離に応じて調整する。",
    "アトラス":      "atlas（テクスチャアトラス） — 複数の小さなテクスチャを1枚の大きなテクスチャにまとめたもの。\nテクスチャ切り替え回数を減らしてドローコールを削減する。スプライトシートとも。",
    "レンダリング":   "rendering — 3D/2Dのデータから最終的な画像を生成する処理全体。\nジオメトリ処理→ラスタライズ→シェーディング→ポストエフェクトの流れ。",
    "ライティング":   "lighting — 光と影の計算。3Dオブジェクトの見た目を決定する重要な処理。\nDirectional/Point/Spotライトとアンビエント光を組み合わせる。PBRが主流。",
    "シェーディング": "shading — 各ピクセルの色を計算する処理。ライティング結果をもとに最終的な見た目を決める。\nPhong、PBR、Toonなどのシェーディングモデルがある。",
    "クリッピング":   "clipping — 描画範囲外のジオメトリを切り捨てる処理。\nビューポート外や近/遠平面外のプリミティブをGPUが自動的にクリップする。",
    "アンチエイリアス":"anti-aliasing（AA） — ジャギー（ギザギザのエッジ）を軽減する技術の総称。\nMSAA（マルチサンプル）、FXAA（高速近似）、TAA（時間方向）などの方式がある。",
    "ディスパッチ":   "dispatch — コンピュートシェーダーの実行命令。\nDispatch(X,Y,Z)でスレッドグループの数を指定してGPU計算を開始する。",
    "トポロジー":     "topology — プリミティブの接続方式。\nTriangleList（独立三角形）、TriangleStrip（連続三角形）、LineList、PointListなど。",
    "テッセレーション":"tessellation — GPUでメッシュのポリゴンを動的に分割して細分化する技術。\nHullシェーダーとDomainシェーダーで制御。近距離で詳細度を上げるのに使う。",
    "サブメッシュ":   "submesh — 1つのメッシュ内で異なるマテリアルが適用される部分。\n人物モデルの肌・服・髪など部位ごとにマテリアルを分ける。",
    "バッチング":     "batching — 複数の描画命令を1つにまとめてドローコール数を減らす最適化。\n同じマテリアルのオブジェクトをまとめるスタティックバッチングが一般的。",
    "ドローコール":   "draw call — CPUからGPUに1つの描画命令を送ること。\n回数が多いとCPUオーバーヘッドが増える。バッチングやインスタンシングで削減する。",
    "コルーチン":     "coroutine — 処理を一時中断し、後から再開できる仕組み。\n待機処理やアニメーションのシーケンスをシンプルに書ける。C++20ではco_await/co_yieldを使う。",
    "リフレクション": "reflection（型情報） — 実行時に型の名前、メンバ、メソッドを動的に取得する仕組み。\nエディタのプロパティ表示、シリアライズの自動化、スクリプトバインディングに使う。\nC++には標準機能がないため、マクロや独自実装で実現する。",
    "ホットリロード": "hot reload — プログラムの実行中にコードやリソースを動的に差し替える機能。\nシェーダーのホットリロードでリアルタイムに見た目を調整できる。開発効率を大幅に向上。",
    // ── Additional shader types ──
    "ハルシェーダー":   "hull shader — テッセレーションの制御を行うシェーダーステージ。\nパッチの分割方法とテッセレーション係数を決定する。ドメインシェーダーと対で使う。",
    "ドメインシェーダー":"domain shader — テッセレーション後の頂点位置を計算するシェーダーステージ。\nハルシェーダーが決めた分割に基づいて、新しい頂点の位置を算出する。",
    "タスクシェーダー": "task shader（増幅シェーダー） — メッシュシェーダーの前段で実行されるシェーダー。\nメッシュレット単位のカリングやLOD選択をGPU上で行い、メッシュシェーダーに渡す。",
    "レイジェネレーションシェーダー":"ray generation shader — レイトレーシングの起点となるシェーダー。\n各ピクセルからレイを飛ばす処理を記述する。TraceRay()を呼んでレイを発射する。",
    "ミスシェーダー":   "miss shader — レイが何にもヒットしなかった時に実行されるシェーダー。\nスカイボックスの色や背景色を返す処理を書く。",
    "ヒットシェーダー": "hit shader（closest hit shader） — レイが最も近いオブジェクトにヒットした時に実行されるシェーダー。\nヒット位置のシェーディング、反射レイの発射などを行う。",
    // ── More compound & specific terms ──
    "アクセラレーション構造":"acceleration structure（AS） — レイトレーシングの高速化に使うBVH（境界ボリューム階層）。\nBLAS（ボトムレベル：メッシュ単位）とTLAS（トップレベル：インスタンス配置）の2階層。",
    "G-Buffer":         "Geometry Buffer — ディファードレンダリングのジオメトリパスで出力されるテクスチャ群。\n法線、アルベド、深度、金属度/粗さなどを各テクスチャに格納し、後からライティングに使う。",
    "MRT":              "Multiple Render Targets — 1回の描画で複数のレンダーターゲットに同時に出力する技術。\nディファードレンダリングのG-Buffer出力で使う。ピクセルシェーダーから複数の出力を書き込む。",
    "MSAA":             "Multi-Sample Anti-Aliasing — ハードウェアベースのアンチエイリアス。\nピクセルの複数のサンプル点でカバレッジを判定し、エッジを滑らかにする。",
    "TLAS":             "Top Level Acceleration Structure — レイトレーシングの最上位BVH。\nBLASのインスタンス配置（位置・回転・スケール）を管理する。毎フレーム動的に更新可能。",
    "BLAS":             "Bottom Level Acceleration Structure — レイトレーシングの下位BVH。\nメッシュのジオメトリ（三角形）を格納する。静的なメッシュは一度構築すれば使い回せる。",
    "SH":               "Spherical Harmonics（球面調和関数） — 球面上の関数を周波数成分で近似する数学的手法。\n低周波の環境光（アンビエント）やライトプローブのデータ圧縮に使う。",
    "ライトプローブ":   "light probe — シーン内の特定の点で周囲の照明環境をキャプチャしたデータ。\nSHやキューブマップで記録し、動的オブジェクトの間接照明に利用する。",
    "キューブマップ":   "cubemap — 6面のテクスチャで全方向を表現するテクスチャ形式。\nスカイボックス、環境マップ反射、ライトプローブに使う。",
    "環境マップ":       "environment map — 周囲の環境を記録したテクスチャ。\n鏡面反射やIBLのソースとして使う。キューブマップや等矩形マップの形式がある。",
    "フレネル":         "Fresnel — 視線角度によって反射率が変化する物理現象。\n斜めから見ると反射が強くなる。PBRレンダリングのSchlick近似で計算する。",
    "AO":               "Ambient Occlusion（環境遮蔽） — 物体の凹みや隙間に発生する柔らかい影。\nSSAO（スクリーンスペース）、RTAO（レイトレーシング）、ベイク済みAOマップなどの手法がある。",
    "POM":              "Parallax Occlusion Mapping — 高さマップからレイマーチングで擬似的な凹凸を表現する技術。\nノーマルマップより立体感があり、実際のジオメトリを増やさずに詳細な表面を表現する。",
    "PCF":              "Percentage Closer Filtering — シャドウマップの影の境界を滑らかにする手法。\n深度比較を複数点で行い、結果を平均化してソフトシャドウを実現する。",
    // ── Engine / Game concepts ──
    "スポーン":         "spawn — ゲームオブジェクトを動的に生成すること。\n敵の出現、弾の発射、パーティクルの生成などに使う。プールから取り出す場合も「スポーン」と呼ぶ。",
    "デスポーン":       "despawn — ゲームオブジェクトを削除/非活性化すること。\n画面外に出た弾や寿命が尽きたパーティクルを除去する。プールに戻す場合もある。",
    "コリジョン":       "collision（衝突） — 2つ以上のオブジェクトが重なること。\n検出（detection）と応答（response）の2段階。ブロードフェーズ→ナローフェーズの順で処理。",
    "ブロードフェーズ": "broad phase — 衝突判定の第1段階。AABB等の簡易判定で衝突の可能性があるペアを高速に絞り込む。\nBVH、グリッド、空間分割などで実装する。",
    "ナローフェーズ":   "narrow phase — 衝突判定の第2段階。ブロードフェーズで残ったペアを正確な形状で詳細判定する。\nGJK、SAT、MPRなどのアルゴリズムで実装する。",
    "セーブデータ":     "save data — ゲームの進行状況を永続化したデータ。\nシリアライズしてファイルに書き出す。チェックポイント、手動セーブ、オートセーブがある。",
    "チェックポイント": "checkpoint — ゲーム進行の自動保存ポイント。\n死亡時やゲームオーバー時にここから再開する。",
    "タイムライン":     "timeline — 時間軸に沿ってイベントやアニメーションを配置・再生する仕組み。\nカットシーンやシーケンシャルなイベント制御に使う。",
    "トゥイーン":       "tween — 始点と終点の間を自動で補間するアニメーション手法。\n位置、色、サイズなどの値を指定時間で変化させる。イージング関数と組み合わせる。",
    "ダイアログ":       "dialogue — ①ゲーム内のキャラクター会話システム。選択肢分岐やフラグ管理を含む。\n②UIダイアログ：ユーザーに確認や入力を求めるポップアップウィンドウ。",
    "ローカライズ":     "localize — ゲームを複数言語に対応させること。\nテキスト、音声、UI配置などを言語別に切り替える。キーベースの翻訳管理が一般的。",
    "アセットパイプライン":"asset pipeline — ゲーム用リソースを制作ツールからエンジンで使える形式に変換する工程。\nインポート→変換→最適化→パッケージングの流れ。",
    "シーングラフ":     "scene graph — シーン内のオブジェクトを親子関係のツリーで管理する構造。\n親の変換が子に伝播する。3Dエンジンの基本的なデータ構造。",
    "フラスタム":       "frustum（視錐台） — カメラの視野を表す四角錐台の3D領域。\nこの中にあるオブジェクトだけが描画対象。フラスタムカリングで範囲外を除外する。",
    // ── Additional technical terms ──
    "アップロードヒープ":"upload heap — CPU→GPUへのデータ転送に使う一時的なメモリ領域。\nテクスチャやバッファの初期データをGPUメモリに送る中継地点。",
    "デフォルトヒープ": "default heap — GPU専用のメモリ領域。最高速だがCPUから直接アクセスできない。\nテクスチャやバッファの本体を配置する。アップロードヒープからコピーで転送する。",
    "リードバック":     "readback — GPUのデータをCPU側に読み戻す操作。\nスクリーンキャプチャ、GPU計算結果の取得、デバッグに使う。速度が遅いため頻繁な使用は避ける。",
    "パイプラインステート":"pipeline state（PSO） — 描画パイプラインの全設定をまとめたオブジェクト。\nシェーダー、ブレンド、深度テスト、ラスタライズ設定を含む。切り替えコストが高いのでソートが重要。",
    "ルートパラメータ": "root parameter — ルートシグネチャの要素。シェーダーに渡すリソースの種類と場所を定義する。\nルート定数、ルートディスクリプタ、ディスクリプタテーブルの3種類。",
    "セマフォ":         "semaphore — カウンタベースの同期プリミティブ。\nリソースへの同時アクセス数を制限する。ミューテックスは同時1つだが、セマフォはN個。",
    "アトミック":       "atomic — マルチスレッド環境で分割不可能な操作を保証する仕組み。\nstd::atomicでロックなしにスレッドセーフな読み書きができる。カウンタやフラグに使う。",
    "キャッシュ":       "cache — ①データの一時保存場所。CPUキャッシュ（L1/L2/L3）は高速メモリ。\n②結果のキャッシュ：計算結果を保存して再利用する最適化。\nキャッシュミスがパフォーマンスに大きく影響する。",
    "フラグメント":     "fragment — ラスタライズで生成されたピクセル候補。\nフラグメントシェーダー（＝ピクセルシェーダー）で色が決まり、深度テスト後に最終ピクセルになる。",
    "テクセル":         "texel（テクスチャピクセル） — テクスチャ上の1つのピクセル。\n画面のピクセルとテクセルの比率がテクスチャの鮮明さに影響する。ミップマップで調整。",
    "ワークグループ":   "work group（スレッドグループ） — コンピュートシェーダーの実行単位。\n通常64〜256スレッドで構成。共有メモリ（groupshared）でスレッド間のデータ交換が可能。",
    "共有メモリ":       "shared memory（groupshared） — コンピュートシェーダーのワークグループ内で共有される高速メモリ。\nスレッド間のデータ交換やタイルベースの最適化に使う。サイズ制限あり（通常32KB）。",
    "レジスタ":         "register — ①シェーダーレジスタ：HLSLでリソースをバインドする番号（t0, b0, u0, s0）。\n②CPUレジスタ：プロセッサ内の最速メモリ。変数はなるべくレジスタに収まるよう最適化される。",
    "スワップ":         "swap — 2つの値を交換する操作。\nstd::swapで実装。ダブルバッファリングのフロント/バックバッファの交換もスワップと呼ぶ。",
    "デリゲート":       "delegate — 関数呼び出しを別のオブジェクトに委譲する仕組み。\nイベントシステムの基盤。C++ではstd::functionやfunctor、関数ポインタで実現する。",
    "タグ":             "tag — オブジェクトに付けるラベル。\n検索やフィルタリングに使う。ECSではタグコンポーネント（データなし）で分類する。",
    "マスク":           "mask — ①ビットマスク：特定のビットだけを抽出/操作するパターン。\n②描画マスク：特定の領域だけ描画を許可/禁止する（ステンシルマスク、テクスチャマスク）。\n③レイヤーマスク：衝突判定や描画のフィルタリング。",
    "クランプ":         "clamp — 値を指定範囲内に制限する操作。\nclamp(x, min, max)でxをmin〜maxの範囲に収める。色値の0〜1制限などに頻繁に使う。",
    "サチュレート":     "saturate — 値を0〜1の範囲にクランプするシェーダー関数。\nHLSLのsaturate(x)はclamp(x, 0, 1)と同じ。色やライティング計算で多用。",
    "ラップ":           "wrap — テクスチャのアドレスモード。UV座標が0〜1を超えた時にテクスチャを繰り返す。\n対義語はクランプ（端の色で固定）やミラー（反転繰り返し）。",
    "ディスクリプタテーブル":"descriptor table — 複数のディスクリプタをまとめたテーブル。\nルートシグネチャのパラメータとして設定し、シェーダーからリソースの配列にアクセスする。",
    "リソースバリア":   "resource barrier — DX12でGPUリソースの状態遷移を宣言する同期命令。\n描画先→シェーダー入力など用途の変更時に必須。適切な配置がパフォーマンスの鍵。",
    "コマンドアロケータ":"command allocator — コマンドリストの命令を格納するメモリ領域。\nGPU実行中はリセットできない。フレームごとに複数用意して使い回す。",
    "コマンドキュー":   "command queue — GPUにコマンドリストを送信する待ち行列。\nDirect（描画+コンピュート）、Compute（コンピュートのみ）、Copy（転送のみ）の3種類。",
    "間接描画":         "indirect draw — GPUバッファから描画パラメータを読み取って描画する方式。\nCPU関与なしにGPUが自律的に描画を制御できる。GPU駆動レンダリングの基盤。",
    "SSGI":             "Screen Space Global Illumination — スクリーン空間で間接照明を近似する手法。\nレイトレーシングなしでGIに近い効果を得られる。SSAOの拡張版とも言える。",
    "ヒストグラム":     "histogram — 値の分布を集計したデータ。\n自動露出（Auto Exposure）で画面の明るさ分布を解析し、適切な露出値を決定する。",
    "露出":             "exposure（露出） — カメラが取り込む光の量。\n自動露出（AutoExposure）で明るいシーンでは暗く、暗いシーンでは明るく自動調整する。",
    "ガンマ":           "gamma — 画面の明るさの非線形特性。\nリニア空間（物理的に正確）とsRGB空間（人間の知覚に合わせた）の変換。\nガンマ補正を正しく行わないと色が不自然になる。",
    "リニア空間":       "linear space — 物理的に正確な明るさの計算空間。\nライティング計算はリニア空間で行い、最終出力時にsRGBに変換する。",
    "sRGB":             "standard RGB — 人間の視覚特性に合わせた色空間。\nモニタの標準色空間。テクスチャは通常sRGB、計算はリニアで行う。",
    "ノーマルマップ":   "normal map（法線マップ） — 表面の凹凸情報をテクスチャに格納した画像。\nRGBの各チャンネルがXYZ方向の法線を表す。低ポリモデルに詳細な質感を与える。",
    "ハイトマップ":     "height map（高さマップ） — グレースケール画像で高さ情報を表現するテクスチャ。\n地形生成、POM（パララックスマッピング）、ディスプレイスメントマッピングに使う。",
    "オパシティ":       "opacity（不透明度） — 物体の不透明さ。1.0で完全不透明、0.0で完全透明。\n半透明描画ではアルファブレンドを使い、描画順序に注意が必要（奥→手前の順）。",
    "アルファ":         "alpha — ①カラーの第4成分（RGBA）。不透明度を表す。\n②アルファテスト：α値が閾値未満のピクセルを破棄（草や鉄柵に使う）。\n③アルファブレンド：半透明の色合成。",
    "エミッシブ":       "emissive（自己発光） — 光源からの照明に関係なく自ら光るマテリアル属性。\nネオンサイン、溶岩、モニタ画面などの表現に使う。Bloomと組み合わせて眩しさを出す。",
    "アンビエントオクルージョン":"ambient occlusion（AO） — 物体の凹みや隙間に溜まる柔らかい影の効果。\nSSAOやベイク済みAOマップで実現し、シーンの奥行き感と写実性を向上させる。",
    "スペキュラマップ": "specular map — 表面の反射強度を定義するテクスチャ。\n場所ごとにハイライトの強さを制御する。PBRではmetallic/roughnessマップが代わりに使われる。",
    "レイマーチング":   "ray marching — レイを少しずつ進めて交差判定する手法。\nSDF（符号付き距離場）と組み合わせてボリューメトリックな表現を行う。雲、霧、POMに使う。",
    "ボリューメトリック":"volumetric — 3D空間の体積を持つ効果の総称。\nボリューメトリックフォグ（霧）、ボリューメトリックライト（光の筋）、ボリューメトリッククラウド（雲）など。",
    "フォグ":           "fog（霧） — 距離に応じてオブジェクトが霞むエフェクト。\nリニア、指数（Exponential）、高さフォグなどの種類がある。奥行き感と雰囲気を演出する。",
    "デプスオブフィールド":"depth of field（DOF/被写界深度） — カメラのピント範囲外をぼかすポストエフェクト。\n焦点距離と絞り値で制御する。映画的な表現に不可欠。",
    "スクリーンスペース":"screen space — 最終的な2D画面上の座標空間。\nSSAO、SSR、SSGIなどスクリーンスペース技法は画面上の情報だけで効果を計算する。高速だが画面外の情報は使えない。",
    // ── Programming paradigms ──
    "データ指向":       "data-oriented design（DOD） — データのメモリレイアウトを重視する設計手法。\nキャッシュ効率を最大化し、大量のオブジェクトを高速に処理する。ECSの基盤思想。\nOOP（オブジェクト指向）の対比として語られる。",
    "コンポジション":   "composition — 機能を「継承」ではなく「持つ（has-a）」で組み合わせる設計。\nコンポーネントシステムの基本思想。柔軟で再利用性が高い。",
    "依存性注入":       "dependency injection（DI） — 必要なオブジェクト（依存）を外部から渡す設計パターン。\nテスタビリティと疎結合を向上させる。ServiceLocatorはDIの一種。",
    "サービスロケータ": "service locator — グローバルなレジストリからサービス（機能提供オブジェクト）を取得するパターン。\nDIの一種。GXLibではGX::ServiceLocator::Instance().Get<T>()でアクセスする。",
    // ── File formats & standards ──
    "FBX":              "Filmbox — Autodeskの3Dモデル交換フォーマット。\nメッシュ、ボーン、アニメーション、マテリアルを含む。ゲーム開発で最も一般的な3Dファイル形式。",
    "glTF":             "GL Transmission Format — Khronos Groupの3Dモデル交換フォーマット。\nJSON+バイナリで構成される。WebやリアルタイムCGの標準形式として普及中。",
    "DXBC":             "DirectX Bytecode — シェーダーモデル5.1以前のコンパイル済みシェーダーバイトコード。\nHLSLをfxcでコンパイルして生成する。",
    "DXIL":             "DirectX Intermediate Language — シェーダーモデル6.0以降のシェーダー中間言語。\nHLSLをdxcでコンパイルして生成する。LLVMベースで最適化が強力。",
    "DDS":              "DirectDraw Surface — GPU向けテクスチャフォーマット。\nBCn圧縮、ミップマップ、キューブマップ、3Dテクスチャを格納できる。",
    // ── Input / UI ──
    "IME":              "Input Method Editor — 日本語や中国語などの入力を補助するシステム。\n変換候補の表示と確定を行う。ゲームのテキスト入力フィールドで対応が必要。",
    "フォーカス":       "focus — 現在の入力対象となっている要素。\nキーボード入力はフォーカスを持つウィジェットに送られる。Tabキーでフォーカスを移動する。",
    "レイアウト":       "layout — UIウィジェットの配置方法。\n垂直（VBox）、水平（HBox）、グリッド、フリー配置などがある。\n親ウィジェットのサイズに応じて子の位置とサイズが自動調整される。",
    "マージン":         "margin — ウィジェットの外側の余白。\n他のウィジェットとの間隔を制御する。CSSと同様にtop/right/bottom/leftで指定。",
    "アンカー":         "anchor — ウィジェットの基準位置。\n親要素のどの位置に固定するかを指定する。画面リサイズ時の追従動作を制御する。",
    "ヒットテスト":     "hit test — マウスやタッチの座標がどのウィジェットの上にあるかを判定する処理。\nクリック判定やホバー判定の基盤。",
    // ── Network ──
    "パケット":         "packet — ネットワークで送受信されるデータの単位。\nヘッダ（宛先情報等）とペイロード（実データ）で構成される。",
    "プロトコル":       "protocol — 通信の手順や約束事。\nTCP、UDP、HTTP、WebSocketなど。ゲームでは独自プロトコルを設計することも多い。",
    "レプリケーション": "replication — マルチプレイでゲーム状態をクライアント間で同期する仕組み。\nサーバーの状態をクライアントに複製（レプリケート）する。",
    "補間":             "interpolation — 離散的なデータ点の間の値を推定する計算。\n線形補間（Lerp）、球面補間（Slerp）、スプライン補間などがある。\nネットワーク同期でのスムージングにも使う。",
    "予測":             "prediction — ネットワーク遅延を隠すために未来の状態を推測する技術。\nクライアント予測：入力をすぐ反映し、サーバー結果で補正する。\nデッドレコニング：他プレイヤーの位置を速度ベースで予測する。",
    "サーバー":         "server — クライアントの要求を処理する中央のコンピュータ。\n権威サーバー（authoritative）が正しいゲーム状態を管理し、不正を防止する。",
    "クライアント":     "client — サーバーに接続して情報を受け取る側のプログラム。\n入力を送信し、サーバーの結果を受信して画面に表示する。",
    "P2P":              "Peer-to-Peer — サーバーを介さず、プレイヤー同士が直接通信する方式。\nサーバー不要だがNAT越えや不正防止が課題。",
    // ── More engine/system terms ──
    "ガベージコレクション":"garbage collection（GC） — 不要になったメモリを自動で回収する仕組み。\nC#やJavaにはあるがC++にはない。C++ではRAIIとスマートポインタで手動管理する。",
    "フラグメンテーション":"fragmentation — メモリの確保・解放を繰り返すことで、空き領域が断片化する現象。\n連続した大きな領域が確保できなくなる。プールアロケータやバディアロケータで防ぐ。",
    "バディアロケータ": "buddy allocator — メモリを2の累乗サイズで分割・統合するアロケータ。\n高速で外部フラグメンテーションが少ない。GPUメモリ管理で使われる。",
    "リングバッファ":   "ring buffer（循環バッファ） — 固定サイズのバッファを循環的に使い回すデータ構造。\n末尾に達したら先頭に戻る。アップロードバッファやログバッファに使う。",
    "ダブルバッファリング":"double buffering — 2つのバッファを交互に使い分ける手法。\n一方に描画中、もう一方を表示。スワップすることで画面のちらつきを防ぐ。",
    "トリプルバッファリング":"triple buffering — 3つのバッファを使う手法。\nVSync有効時にフレームレート低下を抑える。遅延は増えるが滑らかさが向上する。",
    "VSync":            "Vertical Synchronization — 画面のリフレッシュレートと描画のタイミングを同期する技術。\nティアリング（画面のずれ）を防ぐが、フレームレートがリフレッシュレートに制限される。",
    "ティアリング":     "tearing — 画面の上下で異なるフレームが表示される現象。\nVSyncを有効にするか、可変リフレッシュレート（G-Sync/FreeSync）で防ぐ。",
    "ジッター":         "jitter — 時間的な揺らぎやノイズ。\nTAAのサブピクセルジッター（投影行列をわずかにずらす）やネットワークの遅延変動を指す。",
    "アーティファクト": "artifact — 描画の不具合で現れる意図しない視覚的なノイズや乱れ。\nZファイティング、テクスチャの継ぎ目、TAAのゴースティングなど。",
    "Zファイティング": "Z-fighting — 2つのポリゴンがほぼ同じ深度にある時に表示がちらつく現象。\n深度バッファの精度不足が原因。ポリゴンオフセットやニア/ファー面の調整で対処する。",
    // ── Pipeline compound terms (longer → matched before shorter) ──
    "メッシュシェーダーパイプライン":"mesh shader pipeline — メッシュシェーダー＋タスクシェーダーで構成される新世代の描画パイプライン。\n従来の頂点/ジオメトリシェーダーを置き換え、GPU駆動のジオメトリ処理を実現する。",
    "レイトレーシングパイプライン":"ray tracing pipeline — DXRのレイトレーシング専用パイプライン。\nレイジェネレーション、ミス、ヒットシェーダーで構成される。通常の描画パイプラインとは別に定義する。",
    "コンピュートパイプライン":"compute pipeline — コンピュートシェーダーだけで構成されるパイプライン。\n描画ステージを持たず、汎用GPU計算専用。パーティクル、カリング、画像処理に使う。",
    "グラフィックスパイプライン":"graphics pipeline — 頂点→ラスタライズ→ピクセル処理の標準描画パイプライン。\n入力アセンブラ→頂点シェーダー→(テッセレーション→ジオメトリ)→ラスタライザー→ピクセルシェーダー→出力マージャー。",
    "ポストエフェクトパイプライン":"post-effect pipeline — 3D描画後の最終画像に一連のポストエフェクトを順番に適用するパイプライン。\nBloom→SSAO→SSR→TAA→トーンマッピングなどのチェーン。",
    "描画パイプライン": "rendering pipeline — 3Dデータを最終画像に変換する一連の処理ステージ。\n頂点処理、ラスタライズ、ピクセル処理、出力マージの流れ。PSO（Pipeline State Object）で設定する。",
    "オーディオパイプライン":"audio pipeline — 音声データを再生するまでの処理チェーン。\nデコード→エフェクト（リバーブ、EQ）→ミキシング→出力の流れ。バスで階層化する。",
    // ── More compound terms ──
    "インスタンスバッファ":"instance buffer — インスタンシング描画で各インスタンス固有のデータを格納するバッファ。\nワールド行列、色、カスタムデータなどを格納し、同一メッシュの大量描画を効率化する。",
    "頂点バッファ":     "vertex buffer — 頂点データ（位置、法線、UV等）を格納するGPUバッファ。\nインプットアセンブラが読み取り、頂点シェーダーに渡す。",
    "インデックスバッファ":"index buffer — 頂点の接続順序（三角形の構成）を格納するGPUバッファ。\n頂点の重複を避けてメモリ効率を上げる。uint16またはuint32で格納。",
    "定数バッファ":     "constant buffer（CBV） — シェーダーに渡す定数パラメータを格納するバッファ。\n変換行列、マテリアル情報、ライト情報などフレームごとに更新する小さなデータ。",
    "構造化バッファ":   "structured buffer — 同じサイズの要素が並ぶGPUバッファ。\n配列のようにインデックスでアクセスする。コンピュートシェーダーの入出力によく使う。",
    "ダイナミックバッファ":"dynamic buffer — 毎フレーム内容が変わるバッファ。\nアップロードヒープ上に配置してCPUから高速に書き込む。定数バッファや頂点バッファに使う。",
    "深度テスト":       "depth test（深度テスト） — ピクセルの奥行き値をデプスバッファと比較する処理。\n手前のオブジェクトだけを表示し、奥のオブジェクトを隠す。Zテストとも呼ぶ。",
    "ステンシルテスト": "stencil test — ステンシルバッファの値を参照してピクセルの描画を制御するテスト。\nアウトライン描画、ポータル、マスク処理に使う。深度テストの前に実行される。",
    "ブレンドステート": "blend state — 色の合成方法を定義するパイプライン設定。\nソースブレンド、デスティネーションブレンド、ブレンド演算子を指定する。半透明やパーティクルの加算合成に必要。",
    "ラスタライザステート":"rasterizer state — ラスタライズの設定。\nカリングモード（バックフェース/フロントフェース/なし）、塗りつぶしモード（ソリッド/ワイヤーフレーム）、深度バイアスなどを指定。",
    "入力レイアウト":   "input layout — 頂点データのメモリレイアウトを定義する設定。\n各属性（位置、法線、UV等）のフォーマット、オフセット、セマンティクスを指定する。",
    "セマンティクス":   "semantics — HLSLで入出力変数の用途を指定するラベル。\nPOSITION、NORMAL、TEXCOORD、SV_Position、SV_Targetなど。パイプラインのデータの流れを定義する。",
    "ディスプレイスメント":"displacement — 高さマップに基づいて頂点を実際に移動させる技術。\nテッセレーションと組み合わせて使う。POMより正確だが頂点数が増える。",
    "スプラッティング": "splatting — 複数のテクスチャをウェイトマップで混合する手法。\n地形レンダリングで草、土、岩などのテクスチャをブレンドする。",
    "コンタクトシャドウ":"contact shadow — 物体同士が接触する小さな隙間にできる影。\nスクリーンスペースのレイマーチングで計算する。シャドウマップでは表現できない細かい影を補完する。",
    "カスケード":       "cascade — CSM（Cascaded Shadow Maps）の各段階。\n視点からの距離帯ごとに1つのシャドウマップを割り当てる。近い段ほど高精度。",
    "オーバードロー":   "overdraw — 同じピクセルを複数回描画してしまう無駄。\n半透明や深度ソートが不完全な場合に発生。パフォーマンスに直接影響する。",
    "バンディング":     "banding — 色の階調が滑らかに見えず、段々模様が見える現象。\n8ビット色精度の限界。ディザリングやHDR(16bit)で軽減する。",
    "ディザリング":     "dithering — 色の階調の不足を、パターン状のノイズで目立たなくする手法。\nバンディング対策に使う。",
    "ドップラー効果":   "Doppler effect — 音源と聞き手の相対速度で音の高さが変わる物理現象。\n近づくと高く、遠ざかると低く聞こえる。3Dオーディオで再現する。"
};

// Build text-annotation regex — sorted longest first to match longer terms before shorter
var _glossaryTerms = Object.keys(GLOSSARY).sort(function(a, b) { return b.length - a.length; });
var _enTerms = [], _jaTerms = [];
_glossaryTerms.forEach(function(t) {
    if (/^[a-zA-Z]/.test(t)) _enTerms.push(t.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'));
    else _jaTerms.push(t.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'));
});
var _reParts = [];
if (_enTerms.length > 0) _reParts.push("\\b(" + _enTerms.join("|") + ")\\b");
if (_jaTerms.length > 0) _reParts.push("(" + _jaTerms.join("|") + ")");
var GLOSSARY_TEXT_RE = _reParts.length > 0 ? new RegExp(_reParts.join("|"), "g") : null;

function glossifyText(escapedHtml) {
    if (!escapedHtml || !GLOSSARY_TEXT_RE) return escapedHtml;
    GLOSSARY_TEXT_RE.lastIndex = 0;
    return escapedHtml.replace(GLOSSARY_TEXT_RE, function(match) {
        var key = match;
        var tip = GLOSSARY[key];
        if (!tip) {
            var lc = key.toLowerCase();
            for (var k in GLOSSARY) {
                if (k.toLowerCase() === lc) { tip = GLOSSARY[k]; break; }
            }
        }
        if (!tip) return match;
        return '<span class="glossary-term" data-glossary="' + esc(key) + '">' + match + '</span>';
    });
}

function glossaryAttr(term) {
    var tip = GLOSSARY[term];
    if (!tip) return "";
    return ' class="glossary-term" data-glossary="' + esc(term) + '"';
}

function renderQuals(qual) {
    if (!qual) return "";
    var html = "";
    var map = {
        "V": { label: "virtual",  cls: "qual-virtual" },
        "S": { label: "static",   cls: "qual-static" },
        "C": { label: "const",    cls: "qual-const" },
        "P": { label: "pure virtual", cls: "qual-pure" },
        "O": { label: "override", cls: "qual-override" },
        "M": { label: "mutable",  cls: "qual-mutable" },
        "I": { label: "inline",   cls: "qual-inline" }
    };
    for (var i = 0; i < qual.length; i++) {
        var c = qual[i];
        var info = map[c];
        if (!info) continue;
        var glossaryKey = info.label.split(" ")[0];
        html += '<span class="qual ' + info.cls + '" data-glossary="' + esc(glossaryKey) + '">' + info.label + '</span>';
    }
    return html;
}

// ====================================================================
// Syntax Highlighting for Signatures
// ====================================================================

var SIG_KEYWORDS = [
    "virtual", "static", "const", "constexpr", "inline", "explicit",
    "override", "final", "noexcept", "mutable", "volatile",
    "void", "bool", "int", "float", "double", "char", "unsigned", "signed",
    "short", "long", "size_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "int8_t", "int16_t", "int32_t", "int64_t", "auto", "nullptr",
    "true", "false", "default", "delete", "return", "struct", "class", "enum",
    "typename", "template", "public", "protected", "private"
];

// Build a single combined keyword regex (avoids iterating and double-replacing)
var SIG_KW_RE = new RegExp("\\b(" + SIG_KEYWORDS.join("|") + ")\\b", "g");

// ====================================================================
// renderMethodDoc — parse doc string with \n@param / \n@return / \n@note
// ====================================================================

function renderMethodDoc(rawDoc) {
    if (!rawDoc) return "";
    // Split on the double-escaped \n separator (\\\\n in source = \\n in memory)
    var parts = rawDoc.split("\\\\n");
    var desc = "";
    var params = [];
    var returns = [];
    var notes = [];

    for (var i = 0; i < parts.length; i++) {
        var p = parts[i];
        var paramMatch = p.match(/^@param\s+(\S+)\s+(.*)/);
        var returnMatch = p.match(/^@returns?\s+(.*)/);
        var noteMatch = p.match(/^@note\s+(.*)/);
        if (paramMatch) {
            params.push({ name: paramMatch[1], desc: paramMatch[2] });
        } else if (returnMatch) {
            returns.push(returnMatch[1]);
        } else if (noteMatch) {
            notes.push(noteMatch[1]);
        } else {
            if (desc) desc += " ";
            desc += p;
        }
    }

    var html = "";
    if (desc) {
        html += '<div class="member-doc">' + glossifyText(esc(translateDoc(desc))) + '</div>';
    }
    if (params.length > 0) {
        html += '<div class="param-section">';
        for (var j = 0; j < params.length; j++) {
            html += '<div class="param-row"><code>' + esc(params[j].name) + '</code> — ' + glossifyText(esc(translateDoc(params[j].desc))) + '</div>';
        }
        html += '</div>';
    }
    if (returns.length > 0) {
        for (var k = 0; k < returns.length; k++) {
            html += '<div class="return-doc">' + glossifyText(esc(translateDoc(returns[k]))) + '</div>';
        }
    }
    if (notes.length > 0) {
        for (var n = 0; n < notes.length; n++) {
            html += '<div class="note-doc">' + glossifyText(esc(translateDoc(notes[n]))) + '</div>';
        }
    }
    return html;
}

function highlightSig(sig) {
    if (!sig) return "";
    // Escape HTML first
    sig = esc(sig);
    // Use placeholder markers (\x00type\x01text\x02) to avoid HTML-in-HTML conflicts.
    // All replacements produce placeholders; final pass converts to <span>.
    // Numbers
    sig = sig.replace(/\b(\d+\.?\d*f?)\b/g, "\x00num\x01$1\x02");
    // String literals
    sig = sig.replace(/&quot;([^&]*)&quot;/g, "\x00str\x01\"$1\"\x02");
    // Keywords (single pass — no iterating, so "class" in placeholders can't match)
    sig = sig.replace(SIG_KW_RE, "\x00kw\x01$1\x02");
    // Operators
    sig = sig.replace(/(&amp;|&lt;|&gt;|\*|::|-&gt;|=)/g, "\x00op\x01$1\x02");
    // Convert all placeholders to HTML spans
    sig = sig.replace(/\x00(\w+)\x01([^\x02]*)\x02/g, '<span class="sig-$1">$2</span>');
    return sig;
}

// ====================================================================
// Search
// ====================================================================

var searchIndex = [];

function buildSearchIndex() {
    // Index types
    DATA.types.forEach(function(t, idx) {
        searchIndex.push({
            kind: t.kind,
            name: t.name,
            doc: t.doc || "",
            module: t.module,
            file: t.file,
            target: { kind: "type", id: idx }
        });

        // Index methods
        if (t.methods) {
            t.methods.forEach(function(m) {
                if (m.access !== "public") return;
                // Extract method name from signature
                var mname = extractMethodName(m.sig);
                if (mname && mname !== t.name && mname[0] !== "~") {
                    searchIndex.push({
                        kind: "method",
                        name: t.name + "::" + mname,
                        doc: m.doc || "",
                        module: t.module,
                        file: t.file,
                        target: { kind: "type", id: idx }
                    });
                }
            });
        }
    });

    // Index free functions and type aliases
    Object.keys(DATA.files).forEach(function(fp) {
        var f = DATA.files[fp];
        if (f && f.items) {
            f.items.forEach(function(it) {
                if (it.kind === "func") {
                    var fname = extractMethodName(it.sig);
                    if (fname) {
                        searchIndex.push({
                            kind: "func",
                            name: fname,
                            doc: it.doc || "",
                            module: f.module,
                            file: fp,
                            target: { kind: "file", id: fp }
                        });
                    }
                }
            });
        }
    });

    // Index files
    Object.keys(DATA.files).forEach(function(fp) {
        var fname = fp.split("/").pop().replace(".h", "");
        searchIndex.push({
            kind: "file",
            name: fname,
            doc: "",
            module: DATA.files[fp].module,
            file: fp,
            target: { kind: "file", id: fp }
        });
    });
}

function extractMethodName(sig) {
    if (!sig) return "";
    // Remove return type prefix, find name before (
    var parenIdx = sig.indexOf("(");
    if (parenIdx < 0) return "";
    var before = sig.substring(0, parenIdx).trim();
    // Last word before ( is the method name
    var parts = before.split(/\s+/);
    var name = parts[parts.length - 1];
    // Remove leading * or &
    name = name.replace(/^[*&]+/, "");
    // Handle operator overloads
    if (before.indexOf("operator") >= 0) {
        var opIdx = before.indexOf("operator");
        name = before.substring(opIdx, parenIdx).trim();
    }
    return name;
}

function doSearch(query) {
    if (!query || query.length < 2) {
        $searchRes.classList.add("hidden");
        return;
    }

    var q = query.toLowerCase();
    var results = [];

    searchIndex.forEach(function(item) {
        var nameLC = item.name.toLowerCase();
        var score = 0;

        // Exact match
        if (nameLC === q) {
            score = 100;
        }
        // Starts with
        else if (nameLC.indexOf(q) === 0) {
            score = 80;
        }
        // Contains
        else if (nameLC.indexOf(q) >= 0) {
            score = 60;
        }
        // Fuzzy: all chars in order
        else {
            var fi = 0;
            for (var si = 0; si < nameLC.length && fi < q.length; si++) {
                if (nameLC[si] === q[fi]) fi++;
            }
            if (fi === q.length) {
                score = 30;
            }
        }

        // Boost classes/structs/enums over methods
        if (score > 0) {
            if (item.kind === "class" || item.kind === "struct") score += 5;
            else if (item.kind === "enum") score += 3;
            else if (item.kind === "method") score -= 2;

            // Check doc too
            if (item.doc && item.doc.toLowerCase().indexOf(q) >= 0) {
                score += 10;
            }
        }

        if (score > 0) {
            results.push({ item: item, score: score });
        }
    });

    results.sort(function(a,b) { return b.score - a.score; });
    results = results.slice(0, 20);

    if (results.length === 0) {
        $searchRes.innerHTML = '<div class="search-result-item"><span class="sr-name" style="color:var(--text-muted);">「' + esc(query) + '」の検索結果はありません</span></div>';
    } else {
        var html = "";
        results.forEach(function(r, i) {
            var item = r.item;
            var badge = badgeClass(item.kind) || "badge-func";
            var modName = DATA.modules[item.module] ? DATA.modules[item.module].name : "";
            html += '<div class="search-result-item' + (i === 0 ? " active" : "") + '" data-idx="' + i + '" onclick="App._searchSelect(' + i + ')">';
            html += '<span class="sr-badge badge ' + badge + '">' + item.kind + '</span>';
            html += '<span class="sr-name">' + highlightMatch(item.name, query) + '</span>';
            html += '<span class="sr-path">' + esc(modName) + '</span>';
            html += '</div>';
        });
        $searchRes.innerHTML = html;
    }

    $searchRes.classList.remove("hidden");
    window._searchResults = results;
    window._searchActive = 0;
}

function highlightMatch(text, query) {
    var idx = text.toLowerCase().indexOf(query.toLowerCase());
    if (idx < 0) return esc(text);
    return esc(text.substring(0, idx)) + '<mark>' + esc(text.substring(idx, idx + query.length)) + '</mark>' + esc(text.substring(idx + query.length));
}

function searchSelect(idx) {
    var results = window._searchResults;
    if (!results || !results[idx]) return;
    var t = results[idx].item.target;
    closeSearch();
    $searchInput.value = "";
    navigateTo(t.kind, t.id);
}

function closeSearch() {
    $searchRes.classList.add("hidden");
    window._searchResults = null;
}

// ====================================================================
// Mobile Sidebar
// ====================================================================

function toggleSidebar() {
    $sidebar.classList.toggle("open");
    $overlay.classList.toggle("open");
}

function closeSidebar() {
    $sidebar.classList.remove("open");
    $overlay.classList.remove("open");
}

// ====================================================================
// Utility
// ====================================================================

function esc(s) {
    if (!s) return "";
    var div = document.createElement("div");
    div.appendChild(document.createTextNode(s));
    return div.innerHTML;
}

function escAttr(s) {
    return esc(s).replace(/'/g, "\\'");
}

// ====================================================================
// Hash Router
// ====================================================================

function handleHash() {
    var hash = window.location.hash;
    if (!hash || hash === "#" || hash === "#/") {
        navigateTo("home");
        return;
    }

    hash = hash.replace(/^#\/?/, "");
    var parts = hash.split("/");

    if (parts[0] === "module" && parts[1]) {
        navigateTo("module", parts[1]);
    } else if (parts[0] === "file" && parts.length >= 2) {
        var fp = parts.slice(1).join("/");
        navigateTo("file", fp);
    } else if (parts[0] === "type" && parts[1]) {
        navigateTo("type", parseInt(parts[1]));
    } else if (parts[0] === "glossary") {
        navigateTo("glossary", parts[1] || "");
    } else if (parts[0] === "classdict") {
        navigateTo("classdict", parts[1] || "");
    } else {
        navigateTo("home");
    }
}

// ====================================================================
// Event Bindings
// ====================================================================

function init() {
    computeStats();
    buildSidebar();
    buildSearchIndex();

    // Logo click -> home
    $logo.addEventListener("click", function() { navigateTo("home"); });

    // Menu toggle (mobile)
    $menuToggle.addEventListener("click", toggleSidebar);
    $overlay.addEventListener("click", closeSidebar);

    // Search input
    $searchInput.addEventListener("input", function() {
        doSearch(this.value.trim());
    });
    $searchInput.addEventListener("focus", function() {
        if (this.value.trim().length >= 2) doSearch(this.value.trim());
    });

    // Search keyboard nav
    $searchInput.addEventListener("keydown", function(e) {
        var results = window._searchResults;
        var active = window._searchActive || 0;

        if (e.key === "Escape") {
            closeSearch();
            this.blur();
            return;
        }
        if (!results) return;

        if (e.key === "ArrowDown") {
            e.preventDefault();
            active = Math.min(active + 1, results.length - 1);
            window._searchActive = active;
            updateSearchActive(active);
        } else if (e.key === "ArrowUp") {
            e.preventDefault();
            active = Math.max(active - 1, 0);
            window._searchActive = active;
            updateSearchActive(active);
        } else if (e.key === "Enter") {
            e.preventDefault();
            searchSelect(active);
        }
    });

    // Close search on outside click
    document.addEventListener("click", function(e) {
        if (!$searchBox.contains(e.target)) {
            closeSearch();
        }
    });

    // Ctrl+K shortcut
    document.addEventListener("keydown", function(e) {
        if ((e.ctrlKey || e.metaKey) && e.key === "k") {
            e.preventDefault();
            $searchInput.focus();
            $searchInput.select();
        }
        if (e.key === "/" && document.activeElement.tagName !== "INPUT") {
            e.preventDefault();
            $searchInput.focus();
            $searchInput.select();
        }
    });

    // Hash routing
    window.addEventListener("hashchange", handleHash);
    window.addEventListener("popstate", handleHash);
    handleHash();

    // Interactive glossary tooltip system
    initGlossaryTooltip();
}

// ====================================================================
// Interactive Glossary Tooltip
// ====================================================================

var _tooltipCloseAll = function() {};  // set by initGlossaryTooltip
function closeAllTooltips() { _tooltipCloseAll(); }

function initGlossaryTooltip() {
    // Tooltip stack — each entry: { el: DOM element, target: trigger element }
    var tipStack = [];
    var hideTimer = null;
    var BASE_Z = 10000;
    var hoveredElements = new Set();

    _tooltipCloseAll = function() {
        clearTimeout(hideTimer);
        while (tipStack.length > 0) {
            var entry = tipStack.pop();
            hoveredElements.delete(entry.el);
            entry.el.remove();
        }
    };

    function lookupGlossary(key) {
        var tip = GLOSSARY[key];
        if (!tip) {
            var lc = key.toLowerCase();
            for (var k in GLOSSARY) {
                if (k.toLowerCase() === lc) { tip = GLOSSARY[k]; break; }
            }
        }
        return tip || null;
    }

    function createTipEl(level) {
        var el = document.createElement("div");
        el.className = "glossary-popup";
        el.style.zIndex = (BASE_Z + level).toString();
        document.body.appendChild(el);
        return el;
    }

    function positionTip(tipEl, rect) {
        var tipW = tipEl.offsetWidth;
        var tipH = tipEl.offsetHeight;
        var left = rect.left;
        var top = rect.bottom + 6;

        if (left + tipW > window.innerWidth - 12) left = window.innerWidth - tipW - 12;
        if (left < 8) left = 8;
        if (top + tipH > window.innerHeight - 12) {
            top = rect.top - tipH - 6;
        }

        tipEl.style.left = left + "px";
        tipEl.style.top = top + "px";
    }

    // Determine the depth of a target element (0 = page, 1+ = inside tooltip stack)
    function getTargetDepth(target) {
        for (var i = tipStack.length - 1; i >= 0; i--) {
            if (tipStack[i].el.contains(target)) return i + 1;
        }
        return 0;
    }

    // Close all tooltips from depth onward
    function closeFrom(depth) {
        while (tipStack.length > depth) {
            var entry = tipStack.pop();
            hoveredElements.delete(entry.el);
            entry.el.remove();
        }
    }

    function showTip(target) {
        var key = target.getAttribute("data-glossary");
        if (!key) return;
        var tip = lookupGlossary(key);
        if (!tip) return;

        clearTimeout(hideTimer);

        // Save rect BEFORE any DOM mutations
        var rect = target.getBoundingClientRect();

        // Determine at which level this target lives
        var depth = getTargetDepth(target);

        // Close any tooltips deeper than this level (sibling replacement)
        closeFrom(depth);

        // Create new tooltip at this depth
        var tipEl = createTipEl(depth);
        var tipHtml = esc(tip).replace(/\n/g, "<br>");
        tipEl.innerHTML = glossifyText(tipHtml);

        positionTip(tipEl, rect);

        tipStack.push({ el: tipEl, target: target });

        // Attach hover handlers to track mouse presence
        tipEl.addEventListener("mouseenter", function() {
            hoveredElements.add(tipEl);
            clearTimeout(hideTimer);
        });
        tipEl.addEventListener("mouseleave", function() {
            hoveredElements.delete(tipEl);
            scheduleHide();
        });
    }

    function scheduleHide() {
        clearTimeout(hideTimer);
        hideTimer = setTimeout(function() {
            // Find the deepest tooltip the mouse is still inside
            var safeDepth = 0;
            for (var i = 0; i < tipStack.length; i++) {
                if (hoveredElements.has(tipStack[i].el)) {
                    safeDepth = i + 1;
                }
            }
            // Close everything deeper than where the mouse currently is
            closeFrom(safeDepth);
        }, 150);
    }

    // Check if element is inside any tooltip in the stack
    function isInsideTipStack(el) {
        for (var i = 0; i < tipStack.length; i++) {
            if (tipStack[i].el.contains(el) || tipStack[i].el === el) return true;
        }
        return false;
    }

    // Event delegation on document (capture phase)
    document.addEventListener("mouseenter", function(e) {
        // Entering a tooltip element itself — keep alive
        if (isInsideTipStack(e.target) && !e.target.hasAttribute("data-glossary")) {
            clearTimeout(hideTimer);
            return;
        }
        // Entering a glossary term (page-level or inside a tooltip)
        var el = e.target.closest("[data-glossary]");
        if (el) {
            clearTimeout(hideTimer);
            showTip(el);
        }
    }, true);

    document.addEventListener("mouseleave", function(e) {
        // Leaving a glossary term on the page (not inside any tooltip)
        var el = e.target.closest("[data-glossary]");
        if (el && !isInsideTipStack(el) && tipStack.length > 0) {
            scheduleHide();
        }
    }, true);
}

function updateSearchActive(idx) {
    $searchRes.querySelectorAll(".search-result-item").forEach(function(el, i) {
        el.classList.toggle("active", i === idx);
    });
}

// ====================================================================
// Public API
// ====================================================================

window.App = {
    navigateTo: navigateTo,
    toggleModule: toggleModule,
    openModule: openModule,
    _searchSelect: searchSelect
};

// Boot
if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
} else {
    init();
}

})();
