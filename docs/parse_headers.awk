#!/usr/bin/awk -f
# parse_headers.awk — Extract API data from GXLib C++ headers
# Usage: gawk -f parse_headers.awk FILE1.h FILE2.h ...
# Output: ITEM|file|namespace|kind|name|detail lines

BEGIN { depth = 0 }

BEGINFILE {
    depth = 0; in_class = 0; in_enum = 0; in_constns = 0
    class_depth = 0; enum_depth = 0; constns_depth = 0
    access = "public"; class_name = ""; enum_name = ""; constns_name = ""
    doc = ""; ns_depth_count = 0; current_ns = ""; enum_values = ""
    delete ns_names
    pending_constns = 0; pending_constns_name = ""; pending_constns_doc = ""
}

function json_esc(s) {
    gsub(/\\/, "\\\\", s); gsub(/"/, "\\\"", s)
    gsub(/\t/, " ", s); gsub(/\r/, "", s); return s
}
function trim(s) { gsub(/^[ \t]+/, "", s); gsub(/[ \t]+$/, "", s); return s }
function count_char(str, ch,    n, i) {
    n = 0; for (i = 1; i <= length(str); i++) if (substr(str, i, 1) == ch) n++; return n
}
function rebuild_ns(    i, r) {
    r = ""; for (i = 0; i < ns_depth_count; i++) { if (r != "") r = r "::"; r = r ns_names[i] }
    current_ns = r
}
function get_inline_doc(line,    idx) {
    if (line ~ /\/\/\/</) { idx = index(line, "///<"); return trim(substr(line, idx + 4)) }
    if (line ~ /\/\//) { idx = index(line, "//"); return trim(substr(line, idx + 2)) }
    return ""
}
function strip_comment(line,    idx) {
    if (line ~ /\/\//) { idx = index(line, "//"); line = substr(line, 1, idx - 1) }; return line
}
function update_depth(line) {
    depth += count_char(line, "{"); depth -= count_char(line, "}")
}

# Doc comments
/^[ \t]*\/\/\/ @brief / { sub(/^[ \t]*\/\/\/ @brief /, ""); doc = trim($0); next }
/^[ \t]*\/\/\/ / && !/^[ \t]*\/\/\/ @/ {
    sub(/^[ \t]*\/\/\/ ?/, "")
    if (doc != "") doc = doc " " trim($0); else doc = trim($0)
    next
}
# @param name description
/^[ \t]*\/\/\/ @param / {
    sub(/^[ \t]*\/\/\/ /, "")
    if (doc != "") doc = doc "\\n" trim($0); else doc = trim($0)
    next
}
# @return/@returns description
/^[ \t]*\/\/\/ @returns? / {
    sub(/^[ \t]*\/\/\/ /, "")
    if (doc != "") doc = doc "\\n" trim($0); else doc = trim($0)
    next
}
# @note description
/^[ \t]*\/\/\/ @note / {
    sub(/^[ \t]*\/\/\/ /, "")
    if (doc != "") doc = doc "\\n" trim($0); else doc = trim($0)
    next
}
# Skip other @ tags
/^[ \t]*\/\/\/ @/ { next }
/^[ \t]*#/ { next }
/^[ \t]*\/\// && !/^[ \t]*\/\/\// { doc = ""; next }
/^[ \t]*$/ { next }

# ---- Inside class but deeper than class level = method body, skip ----
in_class && depth > class_depth {
    update_depth($0)
    if (depth < class_depth) { in_class = 0; class_name = ""; access = "public" }
    next
}

# ---- Namespace open with brace ----
/^[ \t]*namespace[ \t]+[A-Za-z_][A-Za-z0-9_:]*([ \t]*)\{/ && !in_class {
    ns_line = $0; sub(/^[ \t]*namespace[ \t]+/, "", ns_line)
    sub(/[ \t]*\{.*$/, "", ns_line); ns_line = trim(ns_line)
    # Detect constant namespace: doc must mention flags/constants AND name must not be a plain module ns
    is_cns = (doc ~ /フラグ|ビットフラグ|定数|[Ff]lag|[Cc]onstant/) && (ns_line ~ /[A-Z].*[A-Z]/ || ns_line ~ /[Ff]lag|[Cc]onst|[Kk]ey|[Bb]utton|[Mm]ask|[Ll]imit/)
    n = split(ns_line, nsp, "::")
    for (k = 1; k <= n; k++) { ns_names[ns_depth_count] = nsp[k]; ns_depth_count++ }
    rebuild_ns(); update_depth($0)
    if (is_cns) {
        in_constns = 1; constns_depth = depth; constns_name = ns_line
        printf "ITEM|%s|%s|constns|%s|%s\n", FILENAME, current_ns, constns_name, json_esc(doc)
    }
    doc = ""; next
}
# Namespace without brace
/^[ \t]*namespace[ \t]+[A-Za-z_][A-Za-z0-9_:]*[ \t]*$/ && !in_class {
    ns_line = $0; sub(/^[ \t]*namespace[ \t]+/, "", ns_line); ns_line = trim(ns_line)
    is_cns = (doc ~ /フラグ|ビットフラグ|定数|[Ff]lag|[Cc]onstant/) && (ns_line ~ /[A-Z].*[A-Z]/ || ns_line ~ /[Ff]lag|[Cc]onst|[Kk]ey|[Bb]utton|[Mm]ask|[Ll]imit/)
    n = split(ns_line, nsp, "::")
    for (k = 1; k <= n; k++) { ns_names[ns_depth_count] = nsp[k]; ns_depth_count++ }
    rebuild_ns()
    if (is_cns) { pending_constns = 1; pending_constns_name = ns_line; pending_constns_doc = doc }
    doc = ""; next
}
# Standalone open brace (namespace body or constns body)
/^[ \t]*\{[ \t]*$/ && !in_class && !in_enum {
    update_depth($0)
    if (pending_constns) {
        in_constns = 1; constns_depth = depth; constns_name = pending_constns_name
        printf "ITEM|%s|%s|constns|%s|%s\n", FILENAME, current_ns, constns_name, json_esc(pending_constns_doc)
        pending_constns = 0; pending_constns_name = ""; pending_constns_doc = ""
    }
    next
}

# Helper: parse a comma-separated enum value string and append to enum_values
function parse_enum_vals(valstr, inline_doc,    n, parts, i, v, vn, vi) {
    n = split(valstr, parts, ",")
    for (i = 1; i <= n; i++) {
        v = trim(parts[i])
        if (v == "" || v ~ /^[ \t]*$/) continue
        vn = v; vi = ""
        if (vn ~ /=/) { eq = index(vn, "="); vi = trim(substr(vn, eq+1)); vn = trim(substr(vn, 1, eq-1)) }
        if (enum_values != "") enum_values = enum_values ";"
        enum_values = enum_values json_esc(vn) "=" json_esc(vi) "=" json_esc(i == n ? inline_doc : "")
    }
}

# ---- Enum ----
/^[ \t]*(enum[ \t]+class|enum)[ \t]+[A-Za-z_][A-Za-z0-9_]*/ && !in_class {
    eline = $0
    if (eline ~ /;[ \t]*$/ && eline !~ /\{/) { doc = ""; next }
    sub(/^[ \t]*(enum[ \t]+class|enum)[ \t]+/, "", eline)
    enum_type = ""
    if (eline ~ /:/) {
        split(eline, ep, ":"); enum_name = trim(ep[1])
        et = ep[2]; sub(/[ \t]*\{.*$/, "", et); enum_type = trim(et)
    } else { sub(/[ \t]*\{.*$/, "", eline); enum_name = trim(eline) }
    enum_values = ""
    printf "ITEM|%s|%s|enum|%s|%s|%s\n", FILENAME, current_ns, enum_name, json_esc(doc), json_esc(enum_type)
    # Check if entire enum is on one line (has both { and })
    if ($0 ~ /\{/ && $0 ~ /\}/) {
        # Single-line enum: extract values between { and }
        valstr = $0; sub(/^[^{]*\{/, "", valstr); sub(/\}.*$/, "", valstr)
        parse_enum_vals(valstr, "")
        printf "ITEM|%s|%s|enumvals|%s|%s\n", FILENAME, current_ns, enum_name, enum_values
        enum_name = ""; enum_values = ""
        update_depth($0); doc = ""; next
    }
    in_enum = 1; update_depth($0); enum_depth = depth
    doc = ""; next
}
in_enum {
    update_depth($0)
    if ($0 ~ /\}/) {
        # Extract any values before the closing brace on this line
        valline = $0; sub(/\}.*$/, "", valline)
        idoc = get_inline_doc($0); valline = strip_comment(valline); valline = trim(valline)
        gsub(/^[ \t]*\{[ \t]*/, "", valline); gsub(/,[ \t]*$/, "", valline)
        if (valline != "") parse_enum_vals(valline, idoc)
        in_enum = 0
        printf "ITEM|%s|%s|enumvals|%s|%s\n", FILENAME, current_ns, enum_name, enum_values
        enum_name = ""; enum_values = ""; doc = ""; next
    }
    eline = $0; idoc = get_inline_doc(eline); eline = strip_comment(eline)
    eline = trim(eline); gsub(/,[ \t]*$/, "", eline); gsub(/^[ \t]*\{[ \t]*/, "", eline)
    if (eline != "" && eline !~ /^[ \t]*$/) {
        parse_enum_vals(eline, idoc)
    }
    next
}

# ---- Class / struct ----
/^[ \t]*(class|struct)[ \t]+[A-Za-z_][A-Za-z0-9_<>,: \t]*/ && !in_class && !in_enum {
    cline = $0
    if (cline ~ /;/ && cline !~ /\{/) { doc = ""; next }
    if (cline ~ /;/) { doc = ""; next }
    is_struct = (cline ~ /^[ \t]*struct/)
    sub(/^[ \t]*(class|struct)[ \t]+/, "", cline)
    gsub(/__declspec\([^)]*\)[ \t]*/, "", cline)
    class_bases = ""
    if (cline ~ /[ \t]*:/) {
        colon = index(cline, ":"); cname = trim(substr(cline, 1, colon-1))
        bp = substr(cline, colon+1); sub(/[ \t]*\{.*$/, "", bp); sub(/\/\/.*$/, "", bp)
        class_bases = trim(bp)
    } else { cname = cline }
    sub(/[ \t]*\{.*$/, "", cname); gsub(/[ \t]*final[ \t]*$/, "", cname); cname = trim(cname)
    if (cname == "" || cname ~ /[{}();,]/) { doc = ""; next }
    class_name = cname; in_class = 1; access = is_struct ? "public" : "private"
    has_brace = ($0 ~ /\{/)
    update_depth($0)
    # class_depth = depth at which members reside (inside the { })
    if (has_brace) { class_depth = depth } else { class_depth = depth + 1 }
    printf "ITEM|%s|%s|%s|%s|%s|%s|\n", FILENAME, current_ns, \
        (is_struct ? "struct" : "class"), class_name, json_esc(doc), json_esc(class_bases)
    doc = ""; next
}

# ---- Access specifiers ----
in_class && /^[ \t]*(public|protected|private)[ \t]*:/ {
    if ($0 ~ /public/) access = "public"
    else if ($0 ~ /protected/) access = "protected"
    else access = "private"
    update_depth($0)
    if (depth < class_depth) { in_class = 0; class_name = ""; access = "public" }
    next
}

# ---- Members (inside class, at class depth): unified method/field detection ----
# Strip comments FIRST, then check for parens to decide method vs field
in_class && depth == class_depth && /[;({]/ && !/^[ \t]*(\/\/|#)/ {
    mline_raw = $0; idoc = get_inline_doc(mline_raw)
    mline = strip_comment(mline_raw); mline = trim(mline)

    # Skip irrelevant lines
    if (mline ~ /^friend / || mline ~ /^using / || mline ~ /^typedef / || mline ~ /static_assert/ || mline ~ /^return /) {
        update_depth(mline_raw); doc = ""; next
    }
    # Skip closing brace — must also clear in_class if depth drops
    if (mline ~ /^\}/) {
        update_depth(mline_raw)
        if (depth < class_depth) { in_class = 0; class_name = ""; access = "public" }
        doc = ""; next
    }

    # Determine: method (has parens in code portion) vs field
    code_before_brace = mline; sub(/\{.*$/, "", code_before_brace)
    is_method = (code_before_brace ~ /\(/)

    if (is_method) {
        sig = mline; sub(/[ \t]*\{.*$/, "", sig); sub(/[ \t]*;[ \t]*$/, "", sig)
        sig = trim(sig); gsub(/[ \t]+/, " ", sig)
        if (sig == "") { update_depth(mline_raw); doc = ""; next }
        # Remove initializer lists like : x(x), y(y)
        if (sig ~ /\)[^:]*:/) {
            sub(/\)[^:]*:.*$/, ")", sig)
        }

        qual = ""
        if (sig ~ /^virtual /) qual = qual "V"
        if (sig ~ /^static /) qual = qual "S"
        if (sig ~ /\) *const/) qual = qual "C"
        if (sig ~ /override/) qual = qual "O"
        if (sig ~ /= *0/) qual = qual "P"
        mdoc = doc; if (mdoc == "" && idoc != "") mdoc = idoc
        printf "ITEM|%s|%s|method|%s|%s|%s|%s|%s\n", FILENAME, current_ns, class_name, \
            json_esc(sig), access, json_esc(mdoc), qual
    } else if (mline ~ /;/) {
        fline = mline; sub(/;.*$/, "", fline); fline = trim(fline)
        if (fline == "" || fline ~ /^\}/) { update_depth(mline_raw); doc = ""; next }
        init = ""
        if (fline ~ /=/) { eq = index(fline, "="); init = trim(substr(fline, eq+1)); fline = trim(substr(fline, 1, eq-1)) }
        fqual = ""
        if (fline ~ /^static /) fqual = fqual "S"
        if (fline ~ /const / || fline ~ /constexpr/) fqual = fqual "C"
        if (fline ~ /^mutable /) fqual = fqual "M"
        full = fline; if (init != "") full = full " = " init
        fdoc = doc; if (fdoc == "" && idoc != "") fdoc = idoc
        printf "ITEM|%s|%s|field|%s|%s|%s|%s|%s\n", FILENAME, current_ns, class_name, \
            json_esc(full), access, json_esc(fdoc), fqual
    }
    doc = ""; update_depth(mline_raw)
    if (depth < class_depth) { in_class = 0; class_name = ""; access = "public" }
    next
}

# ---- Constants in namespace ----
in_constns && /constexpr/ {
    cline = $0; idoc = get_inline_doc(cline)
    cline = strip_comment(cline); cline = trim(cline)
    sub(/;[ \t]*$/, "", cline); cline = trim(cline)
    printf "ITEM|%s|%s|const|%s|%s\n", FILENAME, current_ns, json_esc(cline), json_esc(idoc)
    doc = ""; update_depth($0)
    if (depth < constns_depth) { in_constns = 0; constns_name = "" }
    next
}

# ---- Free function ----
!in_class && !in_enum && !in_constns && /^[ \t]*(inline |static )?[A-Za-z_][A-Za-z0-9_:*& <>]*[ \t]+[A-Za-z_~][A-Za-z0-9_]*[ \t]*\(/ {
    fline = $0
    if (fline ~ /^[ \t]*(return|if |for |while |switch )/) { update_depth($0); doc = ""; next }
    sig = strip_comment(fline); sub(/[ \t]*\{.*$/, "", sig); sub(/;[ \t]*$/, "", sig)
    sig = trim(sig); gsub(/[ \t]+/, " ", sig)
    fqual = ""
    if (sig ~ /^static /) fqual = fqual "S"
    if (sig ~ /^inline /) fqual = fqual "I"
    printf "ITEM|%s|%s|func|%s|%s|%s\n", FILENAME, current_ns, json_esc(sig), json_esc(doc), fqual
    doc = ""; update_depth($0); next
}

# ---- Global using alias ----
!in_class && !in_enum && /^[ \t]*using[ \t]+[A-Za-z_]/ && /=/ {
    uline = $0; sub(/;.*$/, "", uline); uline = trim(uline)
    printf "ITEM|%s|%s|using|%s|%s\n", FILENAME, current_ns, json_esc(uline), json_esc(doc)
    doc = ""; update_depth($0); next
}

# ---- Default: track braces ----
{
    update_depth($0)
    if (in_class && depth < class_depth) { in_class = 0; class_name = ""; access = "public" }
    if (in_constns && depth < constns_depth) { in_constns = 0; constns_name = "" }
    while (ns_depth_count > 0 && depth < ns_depth_count) {
        ns_depth_count--; delete ns_names[ns_depth_count]; rebuild_ns()
    }
}
