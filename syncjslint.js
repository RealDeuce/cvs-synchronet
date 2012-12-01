// jslint.js
// 2009-01-26


/*
Copyright (c) 2002 Douglas Crockford  (www.JSLint.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

The Software shall be used for Good, not Evil.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
    JSLINT is a global function. It takes two parameters.

        var myResult = JSLINT(source, option);

    The first parameter is either a string or an array of strings. If it is a
    string, it will be split on '\n' or '\r'. If it is an array of strings, it
    is assumed that each string represents one line. The source can be a
    JavaScript text, or HTML text, or a Konfabulator text.

    The second parameter is an optional object of options which control the
    operation of JSLINT. Most of the options are booleans: They are all are
    optional and have a default value of false.

    If it checks out, JSLINT returns true. Otherwise, it returns false.

    If false, you can inspect JSLINT.errors to find out the problems.
    JSLINT.errors is an array of objects containing these members:

    {
        line      : The line (relative to 0) at which the lint was found
        character : The character (relative to 0) at which the lint was found
        reason    : The problem
        evidence  : The text line in which the problem occurred
        raw       : The raw message before the details were inserted
        a         : The first detail
        b         : The second detail
        c         : The third detail
        d         : The fourth detail
    }

    If a fatal error was found, a null will be the last element of the
    JSLINT.errors array.

    You can request a Function Report, which shows all of the functions
    and the parameters and vars that they use. This can be used to find
    implied global variables and other problems. The report is in HTML and
    can be inserted in an HTML <body>.

        var myReport = JSLINT.report(limited);

    If limited is true, then the report will be limited to only errors.
*/

/*jslint evil: true, nomen: false, onevar: false, regexp: false */

/*global JSLINT*/

/*members "\b", "\t", "\n", "\f", "\r", "\"", "%", "(begin)",
    "(breakage)", "(context)", "(error)", "(global)", "(identifier)",
    "(line)", "(loopage)", "(name)", "(onevar)", "(params)", "(scope)",
    "(verb)", ")", "++", "--", "\/", ADSAFE, Array, Boolean, COM, Canvas,
    CustomAnimation, Date, Debug, E, Error, EvalError, FadeAnimation,
    FormField, Frame, Function, HotKey, Image, JSON, LN10, LN2, LOG10E,
    LOG2E, MAX_VALUE, MIN_VALUE, Math, MenuItem, MoveAnimation,
    NEGATIVE_INFINITY, Number, Object, Option, PI, POSITIVE_INFINITY, Point,
    RangeError, ReferenceError, RegExp, ResizeAnimation, RotateAnimation,
    SQRT1_2, SQRT2, ScrollBar, String, SyntaxError, System, Text, TextArea,
    Timer, TypeError, URIError, URL, Window, XMLDOM, XMLHttpRequest, "\\",
    "]", a, abbr, acronym, active, address, adsafe, after, alert, aliceblue,
    animator, antiquewhite, appleScript, applet, apply, approved, aqua,
    aquamarine, area, arguments, arity, autocomplete, azure, b, background,
    "background-attachment", "background-color", "background-image",
    "background-position", "background-repeat", base, bdo, beep, before,
    beige, big, bisque, bitwise, black, blanchedalmond, block, blockquote,
    blue, blueviolet, blur, body, border, "border-bottom",
    "border-bottom-color", "border-bottom-style", "border-bottom-width",
    "border-collapse", "border-color", "border-left", "border-left-color",
    "border-left-style", "border-left-width", "border-right",
    "border-right-color", "border-right-style", "border-right-width",
    "border-spacing", "border-style", "border-top", "border-top-color",
    "border-top-style", "border-top-width", "border-width", bottom, br,
    brown, browser, burlywood, button, bytesToUIString, c, cadetblue, call,
    callee, caller, canvas, cap, caption, "caption-side", cases, center,
    charAt, charCodeAt, character, chartreuse, chocolate, chooseColor,
    chooseFile, chooseFolder, cite, clear, clearInterval, clearTimeout,
    clip, close, closeWidget, closed, cm, code, col, colgroup, color,
    comment, condition, confirm, console, constructor, content,
    convertPathToHFS, convertPathToPlatform, coral, cornflowerblue,
    cornsilk, "counter-increment", "counter-reset", create, crimson, css,
    cursor, cyan, d, darkblue, darkcyan, darkgoldenrod, darkgray, darkgreen,
    darkkhaki, darkmagenta, darkolivegreen, darkorange, darkorchid, darkred,
    darksalmon, darkseagreen, darkslateblue, darkslategray, darkturquoise,
    darkviolet, dd, debug, decodeURI, decodeURIComponent, deeppink,
    deepskyblue, defaultStatus, defineClass, del, deserialize, dfn, dimgray,
    dir, direction, display, div, dl, document, dodgerblue, dt, else, em,
    embed, empty, "empty-cells", encodeURI, encodeURIComponent, entityify,
    eqeqeq, errors, escape, eval, event, evidence, evil, ex, exec, exps,
    fieldset, filesystem, firebrick, first, "first-child", "first-letter",
    "first-line", float, floor, floralwhite, focus, focusWidget, font,
    "font-face", "font-family", "font-size", "font-size-adjust",
    "font-stretch", "font-style", "font-variant", "font-weight",
    forestgreen, forin, form, fragment, frame, frames, frameset, from,
    fromCharCode, fuchsia, fud, function, g, gainsboro, gc,
    getComputedStyle, ghostwhite, gold, goldenrod, gray, green, greenyellow,
    h1, h2, h3, h4, h5, h6, hasOwnProperty, head, height, help, history,
    honeydew, hotpink, hover, hr, html, i, iTunes, id, identifier, iframe,
    img, import, in, include, indent, indexOf, indianred, indigo, init,
    input, ins, isAlpha, isApplicationRunning, isDigit, isFinite, isNaN,
    ivory, join, kbd, khaki, konfabulatorVersion, label, labelled, lang,
    lavender, lavenderblush, lawngreen, laxbreak, lbp, led, left, legend,
    lemonchiffon, length, "letter-spacing", li, lightblue, lightcoral,
    lightcyan, lightgoldenrodyellow, lightgreen, lightpink, lightsalmon,
    lightseagreen, lightskyblue, lightslategray, lightsteelblue,
    lightyellow, lime, limegreen, line, "line-height", linen, link,
    "list-style", "list-style-image", "list-style-position",
    "list-style-type", load, loadClass, location, log, m, magenta, map,
    margin, "margin-bottom", "margin-left", "margin-right", "margin-top",
    "marker-offset", maroon, match, "max-height", "max-width", media,
    mediumaquamarine, mediumblue, mediumorchid, mediumpurple,
    mediumseagreen, mediumslateblue, mediumspringgreen, mediumturquoise,
    mediumvioletred, menu, message, meta, midnightblue, "min-height",
    "min-width", mintcream, mistyrose, mm, moccasin, moveBy, moveTo, name,
    navajowhite, navigator, navy, new, newcap, noframes, nomen, noscript,
    nud, object, ol, oldlace, olive, olivedrab, on, onblur, onerror, onevar,
    onfocus, onload, onresize, onunload, opacity, open, openURL, opener,
    opera, optgroup, option, orange, orangered, orchid, outer, outline,
    "outline-color", "outline-style", "outline-width", overflow, p, padding,
    "padding-bottom", "padding-left", "padding-right", "padding-top", page,
    palegoldenrod, palegreen, paleturquoise, palevioletred, papayawhip,
    param, parent, parseFloat, parseInt, passfail, pc, peachpuff, peru,
    pink, play, plum, plusplus, pop, popupMenu, position, powderblue, pre,
    predef, preferenceGroups, preferences, print, prompt, prototype, pt,
    purple, push, px, q, quit, quotes, random, range, raw, reach, readFile,
    readUrl, reason, red, regexp, reloadWidget, replace, report, reserved,
    resizeBy, resizeTo, resolvePath, resumeUpdates, rhino, right, rosybrown,
    royalblue, runCommand, runCommandInBg, saddlebrown, safe, salmon, samp,
    sandybrown, saveAs, savePreferences, screen, script, scroll, scrollBy,
    scrollTo, seagreen, seal, search, seashell, select, self, serialize,
    setInterval, setTimeout, shift, showWidgetPreferences, sidebar, sienna,
    silver, skyblue, slateblue, slategray, sleep, slice, small, snow, sort,
    span, spawn, speak, split, springgreen, src, status, steelblue, strict,
    strong, style, styleproperty, sub, substr, sup, supplant,
    suppressUpdates, sync, system, table, "table-layout", tan, tbody, td,
    teal, tellWidget, test, "text-align", "text-decoration", "text-indent",
    "text-shadow", "text-transform", textarea, tfoot, th, thead, thistle,
    title, toLowerCase, toString, toUpperCase, toint32, token, tomato, top,
    tr, tt, turquoise, type, u, ul, undef, unescape, "unicode-bidi",
    unwatch, updateNow, value, valueOf, var, version, "vertical-align",
    violet, visibility, visited, watch, wheat, white, "white-space",
    whitesmoke, widget, width, window, "word-spacing", yahooCheckLogin,
    yahooLogin, yahooLogout, yellow, yellowgreen, "z-index",

    Synchronet,
    log,read,readln,write,write_raw,writeln,print,printf,alert,prompt,
    confirm,exit,load,mswait,yield,random,time,beep,sound,ctrl,ascii,
    ascii_str,strip_ctrl,strip_exascii,truncsp,truncstr,lfexpand,
    wildmatch,backslash,file_getname,file_getext,file_getcase,file_cfgname,
    file_exists,file_remove,file_removecase,file_rename,file_copy,file_backup,
    file_isdir,file_attrib,file_date,file_size,file_utime,file_touch,file_mutex,
    file_compare,directory,dir_freespace,disk_size,socket_select,mkdir,
    rmdir,strftime,format,html_encode,html_decode,word_wrap,quote_msg,
    rot13_translate,base64_encode,base64_decode,crc16_calc,crc32_calc,
    chksum_calc,md5_calc,resolve_ip,resolve_host,netaddr_type,
    list_named_queues,flags_str,
    argc,argv,errno,errnostr,socket_errno,
    global,js,system,server,client,user,bbs,console,msg_area,
    file_area,xtrn_area,MsgBase,File,Queue,Socket,User,

    paranoidbrace, multiload, noradix, noescapement, poorrelations,
    noliteral, noextracomma

    readAll,splice,exec_dir
*/

// We build the application inside a function so that we produce only a single
// global variable. The function will be invoked, its return value is the JSLINT
// application itself.

"use strict";

var JSLINT = function () {
    var adsafe_id,      // The widget's ADsafe id.
        adsafe_may,     // The widget may load approved scripts.
        adsafe_went,    // ADSAFE.go has been called.
        anonname,       // The guessed name for anonymous functions.
        approved,       // ADsafe approved urls.

        atrule = {
            'import'   : true,
            media      : true,
            'font-face': true,
            page       : true
        },

        badbreak = {
            ')': true,
            ']': true,
            '++': true,
            '--': true
        },

// These are members that should not be permitted in third party ads.

        banned = {              // the member names that ADsafe prohibits.
            apply           : true,
            'arguments'     : true,
            call            : true,
            callee          : true,
            caller          : true,
            constructor     : true,
            'eval'          : true,
            prototype       : true,
            unwatch         : true,
            valueOf         : true,
            watch           : true
        },


// These are the JSLint boolean options.

        boolOptions = {
            adsafe     : true, // if ADsafe should be enforced
            bitwise    : true, // if bitwise operators should not be allowed
            browser    : true, // if the standard browser globals should be predefined
            cap        : true, // if upper case HTML should be allowed
            css        : true, // if CSS workarounds should be tolerated
            debug      : true, // if debugger statements should be allowed
            eqeqeq     : true, // if === should be required
            evil       : true, // if eval should be allowed
            forin      : true, // if for in statements must filter
            fragment   : true, // if HTML fragments should be allowed
            laxbreak   : true, // if line breaks should not be checked
            newcap     : true, // if constructor names must be capitalized
            nomen      : true, // if names should be checked
            on         : true, // if HTML event handlers should be allowed
            onevar     : true, // if only one var statement per function should be allowed
            passfail   : true, // if the scan should stop on first error
            plusplus   : true, // if increment/decrement should not be allowed
            regexp     : true, // if the . should not be allowed in regexp literals
            rhino      : true, // if the Rhino environment globals should be predefined
            undef      : true, // if variables should be declared before used
            safe       : true, // if use of some browser features should be restricted
            sidebar    : true, // if the System object should be predefined
            strict     : true, // require the "use strict"; pragma
            sub        : true, // if all forms of subscript notation are tolerated
            white      : true, // if strict whitespace rules apply
            widget     : true, // if the Yahoo Widgets globals should be predefined
			paranoidbrace : true // If we should warn on if(something) doSomething();
        },

// browser contains a set of global names which are commonly provided by a
// web browser environment.

        browser = {
            alert           : true,
            blur            : true,
            clearInterval   : true,
            clearTimeout    : true,
            close           : true,
            closed          : true,
            confirm         : true,
            console         : true,
            Debug           : true,
            defaultStatus   : true,
            document        : true,
            event           : true,
            focus           : true,
            frames          : true,
            getComputedStyle: true,
            history         : true,
            Image           : true,
            length          : true,
            location        : true,
            moveBy          : true,
            moveTo          : true,
            name            : true,
            navigator       : true,
            onblur          : true,
            onerror         : true,
            onfocus         : true,
            onload          : true,
            onresize        : true,
            onunload        : true,
            open            : true,
            opener          : true,
            opera           : true,
            Option          : true,
            parent          : true,
            print           : true,
            prompt          : true,
            resizeBy        : true,
            resizeTo        : true,
            screen          : true,
            scroll          : true,
            scrollBy        : true,
            scrollTo        : true,
            self            : true,
            setInterval     : true,
            setTimeout      : true,
            status          : true,
            top             : true,
            window          : true,
            XMLHttpRequest  : true
        },

        cssAttributeData,
        cssAny,

        cssColorData = {
            "aliceblue": true,
            "antiquewhite": true,
            "aqua": true,
            "aquamarine": true,
            "azure": true,
            "beige": true,
            "bisque": true,
            "black": true,
            "blanchedalmond": true,
            "blue": true,
            "blueviolet": true,
            "brown": true,
            "burlywood": true,
            "cadetblue": true,
            "chartreuse": true,
            "chocolate": true,
            "coral": true,
            "cornflowerblue": true,
            "cornsilk": true,
            "crimson": true,
            "cyan": true,
            "darkblue": true,
            "darkcyan": true,
            "darkgoldenrod": true,
            "darkgray": true,
            "darkgreen": true,
            "darkkhaki": true,
            "darkmagenta": true,
            "darkolivegreen": true,
            "darkorange": true,
            "darkorchid": true,
            "darkred": true,
            "darksalmon": true,
            "darkseagreen": true,
            "darkslateblue": true,
            "darkslategray": true,
            "darkturquoise": true,
            "darkviolet": true,
            "deeppink": true,
            "deepskyblue": true,
            "dimgray": true,
            "dodgerblue": true,
            "firebrick": true,
            "floralwhite": true,
            "forestgreen": true,
            "fuchsia": true,
            "gainsboro": true,
            "ghostwhite": true,
            "gold": true,
            "goldenrod": true,
            "gray": true,
            "green": true,
            "greenyellow": true,
            "honeydew": true,
            "hotpink": true,
            "indianred": true,
            "indigo": true,
            "ivory": true,
            "khaki": true,
            "lavender": true,
            "lavenderblush": true,
            "lawngreen": true,
            "lemonchiffon": true,
            "lightblue": true,
            "lightcoral": true,
            "lightcyan": true,
            "lightgoldenrodyellow": true,
            "lightgreen": true,
            "lightpink": true,
            "lightsalmon": true,
            "lightseagreen": true,
            "lightskyblue": true,
            "lightslategray": true,
            "lightsteelblue": true,
            "lightyellow": true,
            "lime": true,
            "limegreen": true,
            "linen": true,
            "magenta": true,
            "maroon": true,
            "mediumaquamarine": true,
            "mediumblue": true,
            "mediumorchid": true,
            "mediumpurple": true,
            "mediumseagreen": true,
            "mediumslateblue": true,
            "mediumspringgreen": true,
            "mediumturquoise": true,
            "mediumvioletred": true,
            "midnightblue": true,
            "mintcream": true,
            "mistyrose": true,
            "moccasin": true,
            "navajowhite": true,
            "navy": true,
            "oldlace": true,
            "olive": true,
            "olivedrab": true,
            "orange": true,
            "orangered": true,
            "orchid": true,
            "palegoldenrod": true,
            "palegreen": true,
            "paleturquoise": true,
            "palevioletred": true,
            "papayawhip": true,
            "peachpuff": true,
            "peru": true,
            "pink": true,
            "plum": true,
            "powderblue": true,
            "purple": true,
            "red": true,
            "rosybrown": true,
            "royalblue": true,
            "saddlebrown": true,
            "salmon": true,
            "sandybrown": true,
            "seagreen": true,
            "seashell": true,
            "sienna": true,
            "silver": true,
            "skyblue": true,
            "slateblue": true,
            "slategray": true,
            "snow": true,
            "springgreen": true,
            "steelblue": true,
            "tan": true,
            "teal": true,
            "thistle": true,
            "tomato": true,
            "turquoise": true,
            "violet": true,
            "wheat": true,
            "white": true,
            "whitesmoke": true,
            "yellow": true,
            "yellowgreen": true
        },

        cssBorderStyle,

        cssLengthData = {
            '%': true,
            'cm': true,
            'em': true,
            'ex': true,
            'in': true,
            'mm': true,
            'pc': true,
            'pt': true,
            'px': true
        },

        escapes = {
            '\b': '\\b',
            '\t': '\\t',
            '\n': '\\n',
            '\f': '\\f',
            '\r': '\\r',
            '"' : '\\"',
            '/' : '\\/',
            '\\': '\\\\'
        },

        funct,          // The current function
        functions,      // All of the functions

        global,         // The global scope
        htmltag = {
            a:        {},
            abbr:     {},
            acronym:  {},
            address:  {},
            applet:   {},
            area:     {empty: true, parent: ' map '},
            b:        {},
            base:     {empty: true, parent: ' head '},
            bdo:      {},
            big:      {},
            blockquote: {},
            body:     {parent: ' html noframes '},
            br:       {empty: true},
            button:   {},
            canvas:   {parent: ' body p div th td '},
            caption:  {parent: ' table '},
            center:   {},
            cite:     {},
            code:     {},
            col:      {empty: true, parent: ' table colgroup '},
            colgroup: {parent: ' table '},
            dd:       {parent: ' dl '},
            del:      {},
            dfn:      {},
            dir:      {},
            div:      {},
            dl:       {},
            dt:       {parent: ' dl '},
            em:       {},
            embed:    {},
            fieldset: {},
            font:     {},
            form:     {},
            frame:    {empty: true, parent: ' frameset '},
            frameset: {parent: ' html frameset '},
            h1:       {},
            h2:       {},
            h3:       {},
            h4:       {},
            h5:       {},
            h6:       {},
            head:     {parent: ' html '},
            html:     {parent: '*'},
            hr:       {empty: true},
            i:        {},
            iframe:   {},
            img:      {empty: true},
            input:    {empty: true},
            ins:      {},
            kbd:      {},
            label:    {},
            legend:   {parent: ' fieldset '},
            li:       {parent: ' dir menu ol ul '},
            link:     {empty: true, parent: ' head '},
            map:      {},
            menu:     {},
            meta:     {empty: true, parent: ' head noframes noscript '},
            noframes: {parent: ' html body '},
            noscript: {parent: ' body head noframes '},
            object:   {},
            ol:       {},
            optgroup: {parent: ' select '},
            option:   {parent: ' optgroup select '},
            p:        {},
            param:    {empty: true, parent: ' applet object '},
            pre:      {},
            q:        {},
            samp:     {},
            script:   {empty: true, parent: ' body div frame head iframe p pre span '},
            select:   {},
            small:    {},
            span:     {},
            strong:   {},
            style:    {parent: ' head ', empty: true},
            sub:      {},
            sup:      {},
            table:    {},
            tbody:    {parent: ' table '},
            td:       {parent: ' tr '},
            textarea: {},
            tfoot:    {parent: ' table '},
            th:       {parent: ' tr '},
            thead:    {parent: ' table '},
            title:    {parent: ' head '},
            tr:       {parent: ' table tbody thead tfoot '},
            tt:       {},
            u:        {},
            ul:       {},
            'var':    {}
        },

        ids,            // HTML ids
        implied,        // Implied globals
        inblock,
        indent,
        jsonmode,
        lines,
        lookahead,
        member,
        membersOnly,
        nexttoken,
        noreach,
        option,
        predefined,     // Global variables defined by option
        prereg,
        prevtoken,

        pseudorule = {
            'first-child': true,
            link         : true,
            visited      : true,
            hover        : true,
            active       : true,
            focus        : true,
            lang         : true,
            'first-letter' : true,
            'first-line' : true,
            before       : true,
            after        : true
        },

        rhino = {
            defineClass : true,
            deserialize : true,
            gc          : true,
            help        : true,
            load        : true,
            loadClass   : true,
            print       : true,
            quit        : true,
            readFile    : true,
            readUrl     : true,
            runCommand  : true,
            seal        : true,
            serialize   : true,
            spawn       : true,
            sync        : true,
            toint32     : true,
            version     : true
        },

		Synchronet = {
			log			: true,
			read		: true,
			readln		: true,
			write		: true,
			write_raw	: true,
			writeln		: true,
			print		: true,
			printf		: true,
			alert		: true,
			prompt		: true,
			confirm		: true,
			exit		: true,
			load		: true,
			mswait		: true,
			yield		: true,
			random		: true,
			time		: true,
			beep		: true,
			sound		: true,
			ctrl		: true,
			ascii		: true,
			ascii_str	: true,
			strip_ctrl	: true,
			strip_exascii	: true,
			truncsp		: true,
			truncstr	: true,
			lfexpand	: true,
			wildmatch	: true,
			backslash	: true,
			file_getname: true,
			file_getext	: true,
			file_getcase: true,
			file_cfgname: true,
			file_exists	: true,
			file_remove	: true,
			file_removecase	: true,
			file_rename	: true,
			file_copy	: true,
			file_backup	: true,
			file_isdir	: true,
			file_attrib	: true,
			file_date	: true,
			file_size	: true,
			file_utime	: true,
			file_touch	: true,
			file_mutex	: true,
			file_compare: true,
			directory	: true,
			dir_freespace	: true,
			disk_size	: true,
			socket_select	: true,
			mkdir		: true,
			rmdir		: true,
			strftime	: true,
			format		: true,
			html_encode	: true,
			html_decode	: true,
			word_wrap	: true,
			quote_msg	: true,
			rot13_translate	: true,
			base64_encode	: true,
			base64_decode	: true,
			crc16_calc	: true,
			crc32_calc	: true,
			chksum_calc	: true,
			md5_calc	: true,
			resolve_ip	: true,
			resolve_host: true,
			netaddr_type: true,
			list_named_queues	: true,
			flags_str	: true,

			argc		: true,
			argv		: true,
			errno		: true,
			errnostr	: true,
			socket_errno: true,

			global		: true,
			js			: true,
			system		: true,
			server		: true,
			client		: true,
			user		: true,
			bbs			: true,
			console		: true,
			msg_area	: true,
			file_area	: true,
			xtrn_area	: true,
			MsgBase		: true,
			File		: true,
			Queue		: true,
			Socket		: true,
			User		: true
		},

        scope,      // The current scope

        sidebar = {
            System      : true
        },

        src,
        stack,

// standard contains the global names that are provided by the
// ECMAScript standard.

        standard = {
            Array               : true,
            Boolean             : true,
            Date                : true,
            decodeURI           : true,
            decodeURIComponent  : true,
            encodeURI           : true,
            encodeURIComponent  : true,
            Error               : true,
            'eval'              : true,
            EvalError           : true,
            Function            : true,
            isFinite            : true,
            isNaN               : true,
            JSON                : true,
            Math                : true,
            Number              : true,
            Object              : true,
            parseInt            : true,
            parseFloat          : true,
            RangeError          : true,
            ReferenceError      : true,
            RegExp              : true,
            String              : true,
            SyntaxError         : true,
            TypeError           : true,
            URIError            : true
        },

        standard_member = {
            E                   : true,
            LN2                 : true,
            LN10                : true,
            LOG2E               : true,
            LOG10E              : true,
            PI                  : true,
            SQRT1_2             : true,
            SQRT2               : true,
            MAX_VALUE           : true,
            MIN_VALUE           : true,
            NEGATIVE_INFINITY   : true,
            POSITIVE_INFINITY   : true
        },

        syntax = {},
        tab,
        token,
        urls,
        warnings,

// widget contains the global names which are provided to a Yahoo
// (fna Konfabulator) widget.

        widget = {
            alert                   : true,
            animator                : true,
            appleScript             : true,
            beep                    : true,
            bytesToUIString         : true,
            Canvas                  : true,
            chooseColor             : true,
            chooseFile              : true,
            chooseFolder            : true,
            closeWidget             : true,
            COM                     : true,
            convertPathToHFS        : true,
            convertPathToPlatform   : true,
            CustomAnimation         : true,
            escape                  : true,
            FadeAnimation           : true,
            filesystem              : true,
            focusWidget             : true,
            form                    : true,
            FormField               : true,
            Frame                   : true,
            HotKey                  : true,
            Image                   : true,
            include                 : true,
            isApplicationRunning    : true,
            iTunes                  : true,
            konfabulatorVersion     : true,
            log                     : true,
            MenuItem                : true,
            MoveAnimation           : true,
            openURL                 : true,
            play                    : true,
            Point                   : true,
            popupMenu               : true,
            preferenceGroups        : true,
            preferences             : true,
            print                   : true,
            prompt                  : true,
            random                  : true,
            reloadWidget            : true,
            ResizeAnimation         : true,
            resolvePath             : true,
            resumeUpdates           : true,
            RotateAnimation         : true,
            runCommand              : true,
            runCommandInBg          : true,
            saveAs                  : true,
            savePreferences         : true,
            screen                  : true,
            ScrollBar               : true,
            showWidgetPreferences   : true,
            sleep                   : true,
            speak                   : true,
            suppressUpdates         : true,
            system                  : true,
            tellWidget              : true,
            Text                    : true,
            TextArea                : true,
            Timer                   : true,
            unescape                : true,
            updateNow               : true,
            URL                     : true,
            widget                  : true,
            Window                  : true,
            XMLDOM                  : true,
            XMLHttpRequest          : true,
            yahooCheckLogin         : true,
            yahooLogin              : true,
            yahooLogout             : true
        },

//  xmode is used to adapt to the exceptions in html parsing.
//  It can have these states:
//      false   .js script file
//      html
//      outer
//      script
//      style
//      scriptstring
//      styleproperty

        xmode,
        xquote,

// unsafe comment or string
        ax = /@cc|<\/?|script|\]*s\]|<\s*!|&lt/i,
// unsafe characters that are silently deleted by one or more browsers
        cx = /[\u0000-\u001f\u007f-\u009f\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/,
// token
        tx = /^\s*([(){}\[.,:;'"~\?\]#@]|==?=?|\/(\*(global|extern|jslint|member|members)?|=|\/)?|\*[\/=]?|\+[+=]?|-[\-=]?|%=?|&[&=]?|\|[|=]?|>>?>?=?|<([\/=!]|\!(\[|--)?|<=?)?|\^=?|\!=?=?|[a-zA-Z_$][a-zA-Z0-9_$]*|[0-9]+([xX][0-9a-fA-F]+|\.[0-9]*)?([eE][+\-]?[0-9]+)?)/,
// html token
        hx = /^\s*(['"=>\/&#]|<(?:\/|\!(?:--)?)?|[a-zA-Z][a-zA-Z0-9_\-]*|[0-9]+|--|.)/,
// outer html token
        ox = /[>&]|<[\/!]?|--/,
// star slash
        lx = /\*\/|\/\*/,
// identifier
        ix = /^([a-zA-Z_$][a-zA-Z0-9_$]*)$/,
// javascript url
        jx = /^(?:javascript|jscript|ecmascript|vbscript|mocha|livescript)\s*:/i,
// url badness
        ux = /&|\+|\u00AD|\.\.|\/\*|%[^;]|base64|url|expression|data|mailto/i,
// style
        sx = /^\s*([{:#*%.=,>+\[\]@()"';*]|[a-zA-Z0-9_][a-zA-Z0-9_\-]*|<\/|\/\*)/,
        ssx = /^\s*([@#!"'};:\-\/%.=,+\[\]()*_]|[a-zA-Z][a-zA-Z0-9._\-]*|\d+(?:\.\d+)?|<\/)/,
// query characters
        qx = /[\[\]\/\\"'*<>.&:(){}+=#_]/,
// query characters for ids
        dx = /[\[\]\/\\"'*<>.&:(){}+=#]/,

        rx = {
            outer: hx,
            html: hx,
            style: sx,
            styleproperty: ssx
        };

    function F() {}

    if (typeof Object.create !== 'function') {
        Object.create = function (o) {
            F.prototype = o;
            return new F();
        };
    }
    function combine(t, o) {
        var n;
        for (n in o) {
            if (o.hasOwnProperty(n)) {
                t[n] = o[n];
            }
        }
    }

    String.prototype.entityify = function () {
        return this.
            replace(/&/g, '&amp;').
            replace(/</g, '&lt;').
            replace(/>/g, '&gt;');
    };

    String.prototype.isAlpha = function () {
        return (this >= 'a' && this <= 'z\uffff') ||
            (this >= 'A' && this <= 'Z\uffff');
    };


    String.prototype.isDigit = function () {
        return (this >= '0' && this <= '9');
    };


    String.prototype.supplant = function (o) {
        return this.replace(/\{([^{}]*)\}/g, function (a, b) {
            var r = o[b];
            return typeof r === 'string' || typeof r === 'number' ? r : a;
        });
    };

    String.prototype.name = function () {

// If the string looks like an identifier, then we can return it as is.
// If the string contains no control characters, no quote characters, and no
// backslash characters, then we can simply slap some quotes around it.
// Otherwise we must also replace the offending characters with safe
// sequences.


        if (ix.test(this)) {
            return this;
        }
        if (/[&<"\/\\\x00-\x1f]/.test(this)) {
            return '"' + this.replace(/[&<"\/\\\x00-\x1f]/g, function (a) {
                var c = escapes[a];
                if (c) {
                    return c;
                }
                c = a.charCodeAt();
                return '\\u00' +
                    Math.floor(c / 16).toString(16) +
                    (c % 16).toString(16);
            }) + '"';
        }
        return '"' + this + '"';
    };


    function assume() {
    	combine(predefined, Synchronet);
        if (!option.safe) {
            if (option.rhino) {
                combine(predefined, rhino);
            }
            if (option.browser || option.sidebar) {
                combine(predefined, browser);
            }
            if (option.sidebar) {
                combine(predefined, sidebar);
            }
            if (option.widget) {
                combine(predefined, widget);
            }
        }
    }


// Produce an error warning.

    function quit(m, l, ch) {
        throw {
            name: 'JSLintError',
            line: l,
            character: ch,
            message: m + " (" + Math.floor((l / lines.length) * 100) +
                    "% scanned)."
        };
    }

    function warning(m, t, a, b, c, d) {
        var ch, l, w;
        t = t || nexttoken;
        if (t.id === '(end)') {  // `~
            t = token;
        }
        l = t.line || 0;
        ch = t.from || 0;
        w = {
            id: '(error)',
            raw: m,
            evidence: lines[l] || '',
            line: l,
            character: ch,
            a: a,
            b: b,
            c: c,
            d: d
        };
        w.reason = m.supplant(w);
        JSLINT.errors.push(w);
        if (option.passfail) {
            quit('Stopping. ', l, ch);
        }
        warnings += 1;
        if (warnings === 5000) {
            quit("Too many errors.", l, ch);
        }
        return w;
    }

    function warningAt(m, l, ch, a, b, c, d) {
        return warning(m, {
            line: l,
            from: ch
        }, a, b, c, d);
    }

    function error(m, t, a, b, c, d) {
        var w = warning(m, t, a, b, c, d);
        quit("Stopping, unable to continue.", w.line, w.character);
    }

    function errorAt(m, l, ch, a, b, c, d) {
        return error(m, {
            line: l,
            from: ch
        }, a, b, c, d);
    }



// lexical analysis

    var lex = function lex() {
        var character, from, line, s;

// Private lex methods

        function nextLine() {
            var at;
            line += 1;
            if (line >= lines.length) {
                return false;
            }
            character = 0;
            s = lines[line].replace(/\t/g, tab);
            at = s.search(cx);
            if (at >= 0) {
                warningAt("Unsafe character.", line, at);
            }
            return true;
        }

// Produce a token object.  The token inherits from a syntax symbol.

        function it(type, value) {
            var i, t;
            if (type === '(color)') {
                t = {type: type};
            } else if (type === '(punctuator)' ||
                    (type === '(identifier)' && syntax.hasOwnProperty(value))) {
                t = syntax[value] || syntax['(error)'];

// Mozilla bug workaround.

                if (!t.id) {
                    t = syntax[type];
                }
            } else {
                t = syntax[type];
            }
            t = Object.create(t);
            if (type === '(string)' || type === '(range)') {
                if (jx.test(value)) {
                    warningAt("Script URL.", line, from);
                }
            }
            if (type === '(identifier)') {
                t.identifier = true;
                if (option.nomen && value.charAt(0) === '_') {
                    warningAt("Unexpected '_' in '{a}'.", line, from, value);
                }
            }
            t.value = value;
            t.line = line;
            t.character = character;
            t.from = from;
            i = t.id;
            if (i !== '(endline)') {
                prereg = i &&
                        (('(,=:[!&|?{};'.indexOf(i.charAt(i.length - 1)) >= 0) ||
                        i === 'return');
            }
            return t;
        }

// Public lex methods

        return {
            init: function (source) {
                if (typeof source === 'string') {
                    lines = source.
                        replace(/\r\n/g, '\n').
                        replace(/\r/g, '\n').
                        split('\n');
                } else {
                    lines = source;
                }
                line = -1;
                nextLine();
                from = 0;
            },

            range: function (begin, end) {
                var c, value = '';
                from = character;
                if (s.charAt(0) !== begin) {
                    errorAt("Expected '{a}' and instead saw '{b}'.",
                            line, character, begin, s.charAt(0));
                }
                for (;;) {
                    s = s.slice(1);
                    character += 1;
                    c = s.charAt(0);
                    switch (c) {
                    case '':
                        errorAt("Missing '{a}'.", line, character, c);
                        break;
                    case end:
                        s = s.slice(1);
                        character += 1;
                        return it('(range)', value);
                    case xquote:
                    case '\\':
                    case '\'':
                    case '"':
                        warningAt("Unexpected '{a}'.", line, character, c);
                    }
                    value += c;
                }

            },

// token -- this is called by advance to get the next token.

            token: function () {
                var b, c, captures, d, depth, high, i, l, low, q, t;

                function match(x) {
                    var r = x.exec(s), r1;
                    if (r) {
                        l = r[0].length;
                        r1 = r[1];
                        c = r1.charAt(0);
                        s = s.substr(l);
                        character += l;
                        from = character - r1.length;
                        return r1;
                    }
                }

                function string(x) {
                    var c, j, r = '';

                    if (jsonmode && x !== '"') {
                        warningAt("Strings must use doublequote.",
                                line, character);
                    }

                    if (xquote === x || (xmode === 'scriptstring' && !xquote)) {
                        return it('(punctuator)', x);
                    }

                    function esc(n) {
                        var i = parseInt(s.substr(j + 1, n), 16);
                        j += n;
                        if (i >= 32 && i <= 126 &&
                                i !== 34 && i !== 92 && i !== 39) {
                            warningAt("Unnecessary escapement.", line, character);
                        }
                        character += n;
                        c = String.fromCharCode(i);
                    }
                    j = 0;
                    for (;;) {
                        while (j >= s.length) {
                            j = 0;
                            if (xmode !== 'html' || !nextLine()) {
                                errorAt("Unclosed string.", line, from);
                            }
                        }
                        c = s.charAt(j);
                        if (c === x) {
                            character += 1;
                            s = s.substr(j + 1);
                            return it('(string)', r, x);
                        }
                        if (c < ' ') {
                            if (c === '\n' || c === '\r') {
                                break;
                            }
                            warningAt("Control character in string: {a}.",
                                    line, character + j, s.slice(0, j));
                        } else if (c === xquote) {
                            warningAt("Bad HTML string", line, character + j);
                        } else if (c === '<') {
                            if (option.safe && xmode === 'html') {
                                warningAt("ADsafe string violation.",
                                        line, character + j);
                            } else if (s.charAt(j + 1) === '/' && (xmode || option.safe)) {
                                warningAt("Expected '<\\/' and instead saw '</'.", line, character);
                            } else if (s.charAt(j + 1) === '!' && (xmode || option.safe)) {
                                warningAt("Unexpected '<!' in a string.", line, character);
                            }
                        } else if (c === '\\') {
                            if (xmode === 'html') {
                                if (option.safe) {
                                    warningAt("ADsafe string violation.",
                                            line, character + j);
                                }
                            } else if (xmode === 'styleproperty') {
                                j += 1;
                                character += 1;
                                c = s.charAt(j);
                                if (c !== x) {
                                    warningAt("Escapement in style string.",
                                            line, character + j);
                                }
                            } else {
                                j += 1;
                                character += 1;
                                c = s.charAt(j);
                                switch (c) {
                                case xquote:
                                    warningAt("Bad HTML string", line,
                                        character + j);
                                    break;
                                case '\\':
                                case '\'':
                                case '"':
                                case '/':
                                    break;
                                case 'b':
                                    c = '\b';
                                    break;
                                case 'f':
                                    c = '\f';
                                    break;
                                case 'n':
                                    c = '\n';
                                    break;
                                case 'r':
                                    c = '\r';
                                    break;
                                case 't':
                                    c = '\t';
                                    break;
                                case 'u':
                                    esc(4);
                                    break;
                                case 'v':
                                    c = '\v';
                                    break;
                                case 'x':
                                    if (jsonmode) {
                                        warningAt("Avoid \\x-.", line, character);
                                    }
                                    esc(2);
                                    break;
                                default:
                                    if(!option.noescapement) {
                                        warningAt("Bad escapement.", line, character);
                                    }
                                }
                            }
                        }
                        r += c;
                        character += 1;
                        j += 1;
                    }
                }

                for (;;) {
                    if (!s) {
                        return it(nextLine() ? '(endline)' : '(end)', '');
                    }
                    while (xmode === 'outer') {
                        i = s.search(ox);
                        if (i === 0) {
                            break;
                        } else if (i > 0) {
                            character += 1;
                            s = s.slice(i);
                            break;
                        } else {
                            if (!nextLine()) {
                                return it('(end)', '');
                            }
                        }
                    }
                    t = match(rx[xmode] || tx);
                    if (!t) {
                        if (xmode === 'html') {
                            return it('(error)', s.charAt(0));
                        } else {
                            t = '';
                            c = '';
                            while (s && s < '!') {
                                s = s.substr(1);
                            }
                            if (s) {
                                errorAt("Unexpected '{a}'.",
                                        line, character, s.substr(0, 1));
                            }
                        }
                    } else {

    //      identifier

                        if (c.isAlpha() || c === '_' || c === '$') {
                            return it('(identifier)', t);
                        }

    //      number

                        if (c.isDigit()) {
                            if (xmode !== 'style' && !isFinite(Number(t))) {
                                warningAt("Bad number '{a}'.",
                                    line, character, t);
                            }
                            if (xmode !== 'styleproperty' && s.substr(0, 1).isAlpha()) {
                                warningAt("Missing space after '{a}'.",
                                        line, character, t);
                            }
                            if (c === '0') {
                                d = t.substr(1, 1);
                                if (d.isDigit()) {
                                    if (token.id !== '.' && xmode !== 'styleproperty') {
                                        warningAt("Don't use extra leading zeros '{a}'.",
                                            line, character, t);
                                    }
                                } else if (jsonmode && (d === 'x' || d === 'X')) {
                                    warningAt("Avoid 0x-. '{a}'.",
                                            line, character, t);
                                }
                            }
                            if (t.substr(t.length - 1) === '.') {
                                warningAt(
        "A trailing decimal point can be confused with a dot '{a}'.",
                                        line, character, t);
                            }
                            return it('(number)', t);
                        }
                        switch (t) {

    //      string

                        case '"':
                        case "'":
                            return string(t);

    //      // comment

                        case '//':
                            if (src || (xmode && xmode !== 'script')) {
                                warningAt("Unexpected comment.", line, character);
                            } else if (xmode === 'script' && /<\s*\//i.test(s)) {
                                warningAt("Unexpected <\/ in comment.", line, character);
                            } else if ((option.safe || xmode === 'script') && ax.test(s)) {
                                warningAt("Dangerous comment.", line, character);
                            }
                            s = '';
                            token.comment = true;
                            break;

    //      /* comment

                        case '/*':
                            if (src || (xmode && xmode !== 'script' && xmode !== 'style' && xmode !== 'styleproperty')) {
                                warningAt("Unexpected comment.", line, character);
                            }
                            if (option.safe && ax.test(s)) {
                                warningAt("ADsafe comment violation.", line, character);
                            }
                            for (;;) {
                                i = s.search(lx);
                                if (i >= 0) {
                                    break;
                                }
                                if (!nextLine()) {
                                    errorAt("Unclosed comment.", line, character);
                                } else {
                                    if (option.safe && ax.test(s)) {
                                        warningAt("ADsafe comment violation.", line, character);
                                    }
                                }
                            }
                            character += i + 2;
                            if (s.substr(i, 1) === '/') {
                                errorAt("Nested comment.", line, character);
                            }
                            s = s.substr(i + 2);
                            token.comment = true;
                            break;

    //      /*global /*extern /*members /*jslint */

                        case '/*global':
                        case '/*extern':
                        case '/*members':
                        case '/*member':
                        case '/*jslint':
                        case '*/':
                            return {
                                value: t,
                                type: 'special',
                                line: line,
                                character: character,
                                from: from
                            };

                        case '':
                            break;
    //      /
                        case '/':
                            if (prereg) {
                                depth = 0;
                                captures = 0;
                                l = 0;
                                for (;;) {
                                    b = true;
                                    c = s.charAt(l);
                                    l += 1;
                                    switch (c) {
                                    case '':
                                        errorAt("Unclosed regular expression.", line, from);
                                        return;
                                    case '/':
                                        if (depth > 0) {
                                            warningAt("Unescaped '{a}'.", line, from + l, '/');
                                        }
                                        c = s.substr(0, l - 1);
                                        q = {
                                            g: true,
                                            i: true,
                                            m: true
                                        };
                                        while (q[s.charAt(l)] === true) {
                                            q[s.charAt(l)] = false;
                                            l += 1;
                                        }
                                        character += l;
                                        s = s.substr(l);
                                        return it('(regexp)', c);
                                    case '\\':
                                        c = s.charAt(l);
                                        if (c < ' ') {
                                            warningAt("Unexpected control character in regular expression.", line, from + l);
                                        } else if (c === '<') {
                                            warningAt("Unexpected escaped character '{a}' in regular expression.", line, from + l, c);
                                        }
                                        l += 1;
                                        break;
                                    case '(':
                                        depth += 1;
                                        b = false;
                                        if (s.charAt(l) === '?') {
                                            l += 1;
                                            switch (s.charAt(l)) {
                                            case ':':
                                            case '=':
                                            case '!':
                                                l += 1;
                                                break;
                                            default:
                                                warningAt("Expected '{a}' and instead saw '{b}'.", line, from + l, ':', s.charAt(l));
                                            }
                                        } else {
                                            captures += 1;
                                        }
                                        break;
                                    case ')':
                                        if (depth === 0) {
                                            warningAt("Unescaped '{a}'.", line, from + l, ')');
                                        } else {
                                            depth -= 1;
                                        }
                                        break;
                                    case ' ':
                                        q = 1;
                                        while (s.charAt(l) === ' ') {
                                            l += 1;
                                            q += 1;
                                        }
                                        if (q > 1) {
                                            warningAt("Spaces are hard to count. Use {{a}}.", line, from + l, q);
                                        }
                                        break;
                                    case '[':
                                        if (s.charAt(l) === '^') {
                                            l += 1;
                                        }
                                        q = false;
    klass:                              do {
                                            c = s.charAt(l);
                                            l += 1;
                                            switch (c) {
                                            case '[':
                                            case '^':
                                                warningAt("Unescaped '{a}'.", line, from + l, c);
                                                q = true;
                                                break;
                                            case '-':
                                                if (q) {
                                                    q = false;
                                                } else {
                                                    warningAt("Unescaped '{a}'.", line, from + l, '-');
                                                    q = true;
                                                }
                                                break;
                                            case ']':
                                                if (!q) {
                                                    warningAt("Unescaped '{a}'.", line, from + l - 1, '-');
                                                }
                                                break klass;
                                            case '\\':
                                                c = s.charAt(l);
                                                if (c < ' ') {
                                                    warningAt("Unexpected control character in regular expression.", line, from + l);
                                                } else if (c === '<') {
                                                    warningAt("Unexpected escaped character '{a}' in regular expression.", line, from + l, c);
                                                }
                                                l += 1;
                                                q = true;
                                                break;
                                            case '/':
                                                warningAt("Unescaped '{a}'.", line, from + l - 1, '/');
                                                q = true;
                                                break;
                                            case '<':
                                                if (xmode === 'script') {
                                                    c = s.charAt(l);
                                                    if (c === '!' || c === '/') {
                                                        warningAt("HTML confusion in regular expression '<{a}'.", line, from + l, c);
                                                    }
                                                }
                                                q = true;
                                                break;
                                            default:
                                                q = true;
                                            }
                                        } while (c);
                                        break;
                                    case '.':
                                        if (option.regexp) {
                                            warningAt("Unexpected '{a}'.", line, from + l, c);
                                        }
                                        break;
                                    case ']':
                                    case '?':
                                    case '{':
                                    case '}':
                                    case '+':
                                    case '*':
                                        warningAt("Unescaped '{a}'.", line, from + l, c);
                                        break;
                                    case '<':
                                        if (xmode === 'script') {
                                            c = s.charAt(l);
                                            if (c === '!' || c === '/') {
                                                warningAt("HTML confusion in regular expression '<{a}'.", line, from + l, c);
                                            }
                                        }
                                    }
                                    if (b) {
                                        switch (s.charAt(l)) {
                                        case '?':
                                        case '+':
                                        case '*':
                                            l += 1;
                                            if (s.charAt(l) === '?') {
                                                l += 1;
                                            }
                                            break;
                                        case '{':
                                            l += 1;
                                            c = s.charAt(l);
                                            if (c < '0' || c > '9') {
                                                warningAt("Expected a number and instead saw '{a}'.", line, from + l, c);
                                            }
                                            l += 1;
                                            low = +c;
                                            for (;;) {
                                                c = s.charAt(l);
                                                if (c < '0' || c > '9') {
                                                    break;
                                                }
                                                l += 1;
                                                low = +c + (low * 10);
                                            }
                                            high = low;
                                            if (c === ',') {
                                                l += 1;
                                                high = Infinity;
                                                c = s.charAt(l);
                                                if (c >= '0' && c <= '9') {
                                                    l += 1;
                                                    high = +c;
                                                    for (;;) {
                                                        c = s.charAt(l);
                                                        if (c < '0' || c > '9') {
                                                            break;
                                                        }
                                                        l += 1;
                                                        high = +c + (high * 10);
                                                    }
                                                }
                                            }
                                            if (s.charAt(l) !== '}') {
                                                warningAt("Expected '{a}' and instead saw '{b}'.", line, from + l, '}', c);
                                            } else {
                                                l += 1;
                                            }
                                            if (s.charAt(l) === '?') {
                                                l += 1;
                                            }
                                            if (low > high) {
                                                warningAt("'{a}' should not be greater than '{b}'.", line, from + l, low, high);
                                            }
                                        }
                                    }
                                }
                                c = s.substr(0, l - 1);
                                character += l;
                                s = s.substr(l);
                                return it('(regexp)', c);
                            }
                            return it('(punctuator)', t);

    //      punctuator

                        case '#':
                            if (xmode === 'html' || xmode === 'styleproperty') {
                                for (;;) {
                                    c = s.charAt(0);
                                    if ((c < '0' || c > '9') &&
                                            (c < 'a' || c > 'f') &&
                                            (c < 'A' || c > 'F')) {
                                        break;
                                    }
                                    character += 1;
                                    s = s.substr(1);
                                    t += c;
                                }
                                if (t.length !== 4 && t.length !== 7) {
                                    warningAt("Bad hex color '{a}'.", line,
                                        from + l, t);
                                }
                                return it('(color)', t);
                            }
                            return it('(punctuator)', t);
                        default:
                            if (xmode === 'outer' && c === '&') {
                                character += 1;
                                s = s.substr(1);
                                for (;;) {
                                    c = s.charAt(0);
                                    character += 1;
                                    s = s.substr(1);
                                    if (c === ';') {
                                        break;
                                    }
                                    if (!((c >= '0' && c <= '9') ||
                                            (c >= 'a' && c <= 'z') ||
                                            c === '#')) {
                                        errorAt("Bad entity", line, from + l,
                                        character);
                                    }
                                }
                                break;
                            }
                            return it('(punctuator)', t);
                        }
                    }
                }
            }
        };
    }();


    function addlabel(t, type) {

        if (t === 'hasOwnProperty') {
            error("'hasOwnProperty' is a really bad name.");
        }
        if (option.safe && funct['(global)']) {
            warning('ADsafe global: ' + t + '.', token);
        }

// Define t in the current function in the current scope.

        if (funct.hasOwnProperty(t)) {
            warning(funct[t] === true ?
                "'{a}' was used before it was defined." :
                "'{a}' is already defined.",
                nexttoken, t);
        }
        funct[t] = type;
        if (type === 'label') {
            scope[t] = funct;
        } else if (funct['(global)']) {
            global[t] = funct;
            if (implied.hasOwnProperty(t)) {
                warning("'{a}' was used before it was defined.", nexttoken, t);
                delete implied[t];
            }
        } else {
            funct['(scope)'][t] = funct;
        }
    }


    function doOption() {
        var b, obj, filter, o = nexttoken.value, t, v;
        switch (o) {
        case '*/':
            error("Unbegun comment.");
            break;
        case '/*global':
        case '/*extern':
            if (option.safe) {
                warning("ADsafe restriction.");
            }
            obj = predefined;
            break;
        case '/*members':
        case '/*member':
            o = '/*members';
            if (!membersOnly) {
                membersOnly = {};
            }
            obj = membersOnly;
            break;
        case '/*jslint':
            if (option.safe) {
                warning("ADsafe restriction.");
            }
            obj = option;
            filter = boolOptions;
        }
        for (;;) {
            t = lex.token();
            if (t.id === ',') {
                t = lex.token();
            }
            while (t.id === '(endline)') {
                t = lex.token();
            }
            if (t.type === 'special' && t.value === '*/') {
                break;
            }
            if (t.type !== '(string)' && t.type !== '(identifier)' &&
                    o !== '/*members') {
                error("Bad option.", t);
            }
            if (filter) {
                if (filter[t.value] !== true) {
                    error("Bad option.", t);
                }
                v = lex.token();
                if (v.id !== ':') {
                    error("Expected '{a}' and instead saw '{b}'.",
                            t, ':', t.value);
                }
                v = lex.token();
                if (v.value === 'true') {
                    b = true;
                } else if (v.value === 'false') {
                    b = false;
                } else {
                    error("Expected '{a}' and instead saw '{b}'.",
                            t, 'true', t.value);
                }
            } else {
                b = true;
            }
            obj[t.value] = b;
        }
        if (filter) {
            assume();
        }
    }


// We need a peek function. If it has an argument, it peeks that much farther
// ahead. It is used to distinguish
//     for ( var i in ...
// from
//     for ( var i = ...

    function peek(p) {
        var i = p || 0, j = 0, t;

        while (j <= i) {
            t = lookahead[j];
            if (!t) {
                t = lookahead[j] = lex.token();
            }
            j += 1;
        }
        return t;
    }



// Produce the next token. It looks for programming errors.

    function advance(id, t) {
        var l;
        switch (token.id) {
        case '(number)':
            if (nexttoken.id === '.') {
                warning(
"A dot following a number can be confused with a decimal point.", token);
            }
            break;
        case '-':
            if (nexttoken.id === '-' || nexttoken.id === '--') {
                warning("Confusing minusses.");
            }
            break;
        case '+':
            if (nexttoken.id === '+' || nexttoken.id === '++') {
                warning("Confusing plusses.");
            }
            break;
        }
        if (token.type === '(string)' || token.identifier) {
            anonname = token.value;
        }

        if (id && nexttoken.id !== id) {
            if (t) {
                if (nexttoken.id === '(end)') {
                    warning("Unmatched '{a}'.", t, t.id);
                } else {
                    warning("Expected '{a}' to match '{b}' from line {c} and instead saw '{d}'.",
                            nexttoken, id, t.id, t.line + 1, nexttoken.value);
                }
            } else if (nexttoken.type !== '(identifier)' ||
                            nexttoken.value !== id) {
                warning("Expected '{a}' and instead saw '{b}'.",
                        nexttoken, id, nexttoken.value);
            }
        }
        prevtoken = token;
        token = nexttoken;
        for (;;) {
            nexttoken = lookahead.shift() || lex.token();
            if (nexttoken.id === '(end)' || nexttoken.id === '(error)') {
                return;
            }
            if (nexttoken.type === 'special') {
                doOption();
            } else {
                if (nexttoken.id !== '(endline)') {
                    break;
                }
                l = !xmode && !option.laxbreak &&
                    (token.type === '(string)' || token.type === '(number)' ||
                    token.type === '(identifier)' || badbreak[token.id]);
            }
        }
        if (!option.evil && nexttoken.value === 'eval') {
            warning("eval is evil.", nexttoken);
        }
        if (l) {
            switch (nexttoken.id) {
            case '{':
            case '}':
            case ']':
            case '.':
                break;
            case ')':
                switch (token.id) {
                case ')':
                case '}':
                case ']':
                    break;
                default:
                    warning("Line breaking error '{a}'.", token, ')');
                }
                break;
            default:
                warning("Line breaking error '{a}'.",
                        token, token.value);
            }
        }
    }


// This is the heart of JSLINT, the Pratt parser. In addition to parsing, it
// is looking for ad hoc lint patterns. We add to Pratt's model .fud, which is
// like nud except that it is only used on the first token of a statement.
// Having .fud makes it much easier to define JavaScript. I retained Pratt's
// nomenclature.

// .nud     Null denotation
// .fud     First null denotation
// .led     Left denotation
//  lbp     Left binding power
//  rbp     Right binding power

// They are key to the parsing method called Top Down Operator Precedence.

    function parse(rbp, initial) {
        var left, o;
        if (nexttoken.id === '(end)') {
            error("Unexpected early end of program.", token);
        }
        advance();
        if (option.safe && predefined[token.value] === true &&
                (nexttoken.id !== '(' && nexttoken.id !== '.')) {
            warning('ADsafe violation.', token);
        }
        if (initial) {
            anonname = 'anonymous';
            funct['(verb)'] = token.value;
        }
        if (initial === true && token.fud) {
            left = token.fud();
        } else {
            if (token.nud) {
                o = token.exps;
                left = token.nud();
            } else {
                if (nexttoken.type === '(number)' && token.id === '.') {
                    warning(
"A leading decimal point can be confused with a dot: '.{a}'.",
                            token, nexttoken.value);
                    advance();
                    return token;
                } else {
                    error("Expected an identifier and instead saw '{a}'.",
                            token, token.id);
                }
            }
            while (rbp < nexttoken.lbp) {
                o = nexttoken.exps;
                advance();
                if (token.led) {
                    left = token.led(left);
                } else {
                    error("Expected an operator and instead saw '{a}'.",
                        token, token.id);
                }
            }
            if (initial && !o) {
                warning(
"Expected an assignment or function call and instead saw an expression.",
                        token);
            }
        }
        return left;
    }


// Functions for conformance of style.

    function abut(left, right) {
        left = left || token;
        right = right || nexttoken;
        if (left.line !== right.line || left.character !== right.from) {
            warning("Unexpected space after '{a}'.", right, left.value);
        }
    }


    function adjacent(left, right) {
        left = left || token;
        right = right || nexttoken;
        if (option.white || xmode === 'styleproperty' || xmode === 'style') {
            if (left.character !== right.from && left.line === right.line) {
                warning("Unexpected space after '{a}'.", right, left.value);
            }
        }
    }


    function nospace(left, right) {
        left = left || token;
        right = right || nexttoken;
        if (option.white && !left.comment) {
            if (left.line === right.line) {
                adjacent(left, right);
            }
        }
    }


    function nonadjacent(left, right) {
        left = left || token;
        right = right || nexttoken;
        if (option.white) {
            if (left.character === right.from) {
                warning("Missing space after '{a}'.",
                        nexttoken, left.value);
            }
        }
    }

    function indentation(bias) {
        var i;
        if (option.white && nexttoken.id !== '(end)') {
            i = indent + (bias || 0);
            if (nexttoken.from !== i) {
                warning("Expected '{a}' to have an indentation of {b} instead of {c}.",
                        nexttoken, nexttoken.value, i, nexttoken.from);
            }
        }
    }

    function nolinebreak(t) {
        if (t.line !== nexttoken.line) {
            warning("Line breaking error '{a}'.", t, t.id);
        }
    }


// Parasitic constructors for making the symbols that will be inherited by
// tokens.

    function symbol(s, p) {
        var x = syntax[s];
        if (!x || typeof x !== 'object') {
            syntax[s] = x = {
                id: s,
                lbp: p,
                value: s
            };
        }
        return x;
    }


    function delim(s) {
        return symbol(s, 0);
    }


    function stmt(s, f) {
        var x = delim(s);
        x.identifier = x.reserved = true;
        x.fud = f;
        return x;
    }


    function blockstmt(s, f) {
        var x = stmt(s, f);
        x.block = true;
        return x;
    }


    function reserveName(x) {
        var c = x.id.charAt(0);
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            x.identifier = x.reserved = true;
        }
        return x;
    }


    function prefix(s, f) {
        var x = symbol(s, 150);
        reserveName(x);
        x.nud = (typeof f === 'function') ? f : function () {
            if (option.plusplus && (this.id === '++' || this.id === '--')) {
                warning("Unexpected use of '{a}'.", this, this.id);
            }
            this.right = parse(150);
            this.arity = 'unary';
            return this;
        };
        return x;
    }


    function type(s, f) {
        var x = delim(s);
        x.type = s;
        x.nud = f;
        return x;
    }


    function reserve(s, f) {
        var x = type(s, f);
        x.identifier = x.reserved = true;
        return x;
    }


    function reservevar(s, v) {
        return reserve(s, function () {
            if (this.id === 'this') {
                if (option.safe) {
                    warning("ADsafe violation.", this);
                }
            }
            return this;
        });
    }


    function infix(s, f, p) {
        var x = symbol(s, p);
        reserveName(x);
        x.led = (typeof f === 'function') ? f : function (left) {
            nonadjacent(prevtoken, token);
            nonadjacent(token, nexttoken);
            this.left = left;
            this.right = parse(p);
            return this;
        };
        return x;
    }


    function relation(s, f) {
        var x = symbol(s, 100);
        x.led = function (left) {
            nonadjacent(prevtoken, token);
            nonadjacent(token, nexttoken);
            var right = parse(100);
            if ((left && left.id === 'NaN') || (right && right.id === 'NaN')) {
                warning("Use the isNaN function to compare with NaN.", this);
            } else if (f) {
                f.apply(this, [left, right]);
            }
            this.left = left;
            this.right = right;
            return this;
        };
        return x;
    }


    function isPoorRelation(node) {
	if(option.poorrelations)
            return(false);
        return (node.type === '(number)' && !+node.value) ||
               (node.type === '(string)' && !node.value) ||
                node.type === 'true' ||
                node.type === 'false' ||
                node.type === 'undefined' ||
                node.type === 'null';
    }


    function assignop(s, f) {
        symbol(s, 20).exps = true;
        return infix(s, function (left) {
            var l;
            this.left = left;
            nonadjacent(prevtoken, token);
            nonadjacent(token, nexttoken);
            if (option.safe) {
                l = left;
                do {
                    if (predefined[l.value] === true) {
                        warning('ADsafe violation.', l);
                    }
                    l = l.left;
                } while (l);
            }
            if (left) {
                if (left.id === '.' || left.id === '[') {
                    if (left.left.value === 'arguments') {
                        warning('Bad assignment.', this);
                    }
                    this.right = parse(19);
                    return this;
                } else if (left.identifier && !left.reserved) {
                    this.right = parse(19);
                    return this;
                }
                if (left === syntax['function']) {
                    warning(
"Expected an identifier in an assignment and instead saw a function invocation.",
                                token);
                }
            }
            error("Bad assignment.", this);
        }, 20);
    }

    function bitwise(s, f, p) {
        var x = symbol(s, p);
        reserveName(x);
        x.led = (typeof f === 'function') ? f : function (left) {
            if (option.bitwise) {
                warning("Unexpected use of '{a}'.", this, this.id);
            }
            nonadjacent(prevtoken, token);
            nonadjacent(token, nexttoken);
            this.left = left;
            this.right = parse(p);
            return this;
        };
        return x;
    }

    function bitwiseassignop(s) {
        symbol(s, 20).exps = true;
        return infix(s, function (left) {
            if (option.bitwise) {
                warning("Unexpected use of '{a}'.", this, this.id);
            }
            nonadjacent(prevtoken, token);
            nonadjacent(token, nexttoken);
            if (left) {
                if (left.id === '.' || left.id === '[' ||
                        (left.identifier && !left.reserved)) {
                    parse(19);
                    return left;
                }
                if (left === syntax['function']) {
                    warning(
"Expected an identifier in an assignment, and instead saw a function invocation.",
                                token);
                }
            }
            error("Bad assignment.", this);
        }, 20);
    }


    function suffix(s, f) {
        var x = symbol(s, 150);
        x.led = function (left) {
            if (option.plusplus) {
                warning("Unexpected use of '{a}'.", this, this.id);
            }
            this.left = left;
            return this;
        };
        return x;
    }


    function optionalidentifier() {
        if (nexttoken.reserved) {
            warning("Expected an identifier and instead saw '{a}' (a reserved word).",
                    nexttoken, nexttoken.id);
        }
        if (nexttoken.identifier) {
            advance();
            return token.value;
        }
    }


    function identifier() {
        var i = optionalidentifier();
        if (i) {
            return i;
        }
        if (token.id === 'function' && nexttoken.id === '(') {
            warning("Missing name in function statement.");
        } else {
            error("Expected an identifier and instead saw '{a}'.",
                    nexttoken, nexttoken.value);
        }
    }

    function reachable(s) {
        var i = 0, t;
        if (nexttoken.id !== ';' || noreach) {
            return;
        }
        for (;;) {
            t = peek(i);
            if (t.reach) {
                return;
            }
            if (t.id !== '(endline)') {
                if (t.id === 'function') {
                    warning(
"Inner functions should be listed at the top of the outer function.", t);
                    break;
                }
                warning("Unreachable '{a}' after '{b}'.", t, t.value, s);
                break;
            }
            i += 1;
        }
    }


    function statement(noindent) {
        var i = indent, r, s = scope, t = nexttoken;

// We don't like the empty statement.

        if (t.id === ';') {
            warning("Unnecessary semicolon.", t);
            advance(';');
            return;
        }

// Is this a labelled statement?

        if (t.identifier && !t.reserved && peek().id === ':') {
            advance();
            advance(':');
            scope = Object.create(s);
            addlabel(t.value, 'label');
            if (!nexttoken.labelled) {
                warning("Label '{a}' on {b} statement.",
                        nexttoken, t.value, nexttoken.value);
            }
            if (jx.test(t.value + ':')) {
                warning("Label '{a}' looks like a javascript url.",
                        t, t.value);
            }
            nexttoken.label = t.value;
            t = nexttoken;
        }

// Parse the statement.

        if (!noindent) {
            indentation();
        }
        r = parse(0, true);

// Look for the final semicolon.

        if (!t.block) {
            if (nexttoken.id !== ';') {
                warningAt("Missing semicolon.", token.line,
                        token.from + token.value.length);
            } else {
                adjacent(token, nexttoken);
                advance(';');
                nonadjacent(token, nexttoken);
            }
        }

// Restore the indentation.

        indent = i;
        scope = s;
        return r;
    }


    function statements(begin) {
        var a = [];
        if (begin) {
            if (option.strict && nexttoken.type !== '(string)') {
                warning('Missing "use strict" statement.',
                    nexttoken);
            }
            if (nexttoken.type === '(string)' &&
                    nexttoken.value === 'use strict') {
                advance();
                advance(';');
            }
        }
        if (option.adsafe) {
            switch (begin) {
            case 'script':
                if (!adsafe_may) {
                    if (nexttoken.value !== 'ADSAFE' ||
                            peek(0).id !== '.' ||
                            (peek(1).value !== 'id' &&
                            peek(1).value !== 'go')) {
                        error('ADsafe violation: Missing ADSAFE.id or ADSAFE.go.',
                            nexttoken);
                    }
                }
                if (nexttoken.value === 'ADSAFE' &&
                        peek(0).id === '.' &&
                        peek(1).value === 'id') {
                    if (adsafe_may) {
                        error('ADsafe violation.', nexttoken);
                    }
                    advance('ADSAFE');
                    advance('.');
                    advance('id');
                    advance('(');
                    if (nexttoken.value !== adsafe_id) {
                        error('ADsafe violation: id does not match.', nexttoken);
                    }
                    advance('(string)');
                    advance(')');
                    advance(';');
                    adsafe_may = true;
                }
                break;
            case 'lib':
                if (nexttoken.value === 'ADSAFE') {
                    advance('ADSAFE');
                    advance('.');
                    advance('lib');
                    advance('(');
                    advance('(string)');
                    advance(',');
                    parse(0);
                    advance(')');
                    advance(';');
                    return a;
                } else {
                    error("ADsafe lib violation.");
                }
            }
        }
        while (!nexttoken.reach && nexttoken.id !== '(end)') {
            if (nexttoken.id === ';') {
                warning("Unnecessary semicolon.");
                advance(';');
            } else {
                a.push(statement());
            }
        }
        return a;
    }


    function block(f) {
        var a, b = inblock, s = scope, t;
        inblock = f;
        if (f) {
            scope = Object.create(scope);
        }
        nonadjacent(token, nexttoken);
        t = nexttoken;
        if (nexttoken.id === '{') {
            advance('{');
            if (nexttoken.id !== '}' || token.line !== nexttoken.line) {
                indent += option.indent;
                if (!f && nexttoken.from === indent + option.indent) {
                    indent += option.indent;
                }
                a = statements();
                indent -= option.indent;
                indentation();
            }
            advance('}', t);
        } else {
			if(option.paranoidbrace) {
        	    warning("Expected '{a}' and instead saw '{b}'.",
            	        nexttoken, '{', nexttoken.value);
			}
            noreach = true;
            a = [statement()];
            noreach = false;
        }
        funct['(verb)'] = null;
        scope = s;
        inblock = b;
        return a;
    }


// An identity function, used by string and number tokens.

    function idValue() {
        return this;
    }


    function countMember(m) {
        if (membersOnly && membersOnly[m] !== true) {
            warning("Unexpected /*member '{a}'.", nexttoken, m);
        }
        if (typeof member[m] === 'number') {
            member[m] += 1;
        } else {
            member[m] = 1;
        }
    }

    function note_implied(token) {
        var name = token.value, line = token.line + 1, a = implied[name];
        if (!a) {
            a = [line];
            implied[name] = a;
        } else if (a[a.length - 1] !== line) {
            a.push(line);
        }
    }

// CSS parsing.


    function cssName() {
        if (nexttoken.identifier) {
            advance();
            return true;
        }
    }

    function cssNumber() {
        if (nexttoken.id === '-') {
            advance('-');
            advance('(number)');
        }
        if (nexttoken.type === '(number)') {
            advance();
            return true;
        }
    }

    function cssString() {
        if (nexttoken.type === '(string)') {
            advance();
            return true;
        }
    }

    function cssColor() {
        var i, number;
        if (nexttoken.identifier) {
            if (nexttoken.value === 'rgb') {
                advance();
                advance('(');
                for (i = 0; i < 3; i += 1) {
                    number = nexttoken.value;
                    if (nexttoken.type !== '(number)' || number < 0) {
                        warning("Expected a positive number and instead saw '{a}'",
                            nexttoken, number);
                        advance();
                    } else {
                        advance();
                        if (nexttoken.id === '%') {
                            advance('%');
                            if (number > 100) {
                                warning("Expected a percentage and instead saw '{a}'",
                                    token, number);
                            }
                        } else {
                            if (number > 255) {
                                warning("Expected a small number and instead saw '{a}'",
                                    token, number);
                            }
                        }
                    }
                }
                advance(')');
                return true;
            } else if (cssColorData[nexttoken.value] === true) {
                advance();
                return true;
            }
        } else if (nexttoken.type === '(color)') {
            advance();
            return true;
        }
        return false;
    }

    function cssLength() {
        if (nexttoken.id === '-') {
            advance('-');
            adjacent();
        }
        if (nexttoken.type === '(number)') {
            advance();
            if (nexttoken.type !== '(string)' &&
                    cssLengthData[nexttoken.value] === true) {
                adjacent();
                advance();
            } else if (+token.value !== 0) {
                warning("Expected a linear unit and instead saw '{a}'.",
                    nexttoken, nexttoken.value);
            }
            return true;
        }
        return false;
    }

    function cssLineHeight() {
        if (nexttoken.id === '-') {
            advance('-');
            adjacent();
        }
        if (nexttoken.type === '(number)') {
            advance();
            if (nexttoken.type !== '(string)' &&
                    cssLengthData[nexttoken.value] === true) {
                adjacent();
                advance();
            }
            return true;
        }
        return false;
    }

    function cssWidth() {
        if (nexttoken.identifier) {
            switch (nexttoken.value) {
            case 'thin':
            case 'medium':
            case 'thick':
                advance();
                return true;
            }
        } else {
            return cssLength();
        }
    }

    function cssMargin() {
        if (nexttoken.identifier) {
            if (nexttoken.value === 'auto') {
                advance();
                return true;
            }
        } else {
            return cssLength();
        }
    }

    function cssAttr() {
        if (nexttoken.identifier && nexttoken.value === 'attr') {
            advance();
            advance('(');
            if (!nexttoken.identifier) {
                warning("Expected a name and instead saw '{a}'.",
                        nexttoken, nexttoken.value);
            }
            advance();
            advance(')');
            return true;
        }
        return false;
    }

    function cssCommaList() {
        while (nexttoken.id !== ';') {
            if (!cssName() && !cssString()) {
                warning("Expected a name and instead saw '{a}'.",
                        nexttoken, nexttoken.value);
            }
            if (nexttoken.id !== ',') {
                return true;
            }
            advance(',');
        }
    }

    function cssCounter() {
        if (nexttoken.identifier && nexttoken.value === 'counter') {
            advance();
            advance('(');
            if (!nexttoken.identifier) {
            }
            advance();
            if (nexttoken.id === ',') {
                advance(',');
                if (nexttoken.type !== '(string)') {
                    warning("Expected a string and instead saw '{a}'.",
                        nexttoken, nexttoken.value);
                }
                advance();
            }
            advance(')');
            return true;
        }
        if (nexttoken.identifier && nexttoken.value === 'counters') {
            advance();
            advance('(');
            if (!nexttoken.identifier) {
                warning("Expected a name and instead saw '{a}'.",
                        nexttoken, nexttoken.value);
            }
            advance();
            if (nexttoken.id === ',') {
                advance(',');
                if (nexttoken.type !== '(string)') {
                    warning("Expected a string and instead saw '{a}'.",
                        nexttoken, nexttoken.value);
                }
                advance();
            }
            if (nexttoken.id === ',') {
                advance(',');
                if (nexttoken.type !== '(string)') {
                    warning("Expected a string and instead saw '{a}'.",
                        nexttoken, nexttoken.value);
                }
                advance();
            }
            advance(')');
            return true;
        }
        return false;
    }


    function cssShape() {
        var i;
        if (nexttoken.identifier && nexttoken.value === 'rect') {
            advance();
            advance('(');
            for (i = 0; i < 4; i += 1) {
                if (!cssLength()) {
                    warning("Expected a number and instead saw '{a}'.",
                        nexttoken, nexttoken.value);
                    break;
                }
            }
            advance(')');
            return true;
        }
        return false;
    }

    function cssUrl() {
        var url;
        if (nexttoken.identifier && nexttoken.value === 'url') {
            nexttoken = lex.range('(', ')');
            url = nexttoken.value;
            advance();
            if (option.safe && ux.test(url)) {
                error("ADsafe URL violation.");
            }
            urls.push(url);
            return true;
        }
        return false;
    }

    cssAny = [cssUrl, function () {
        for (;;) {
            if (nexttoken.identifier) {
                switch (nexttoken.value.toLowerCase()) {
                case 'url':
                    cssUrl();
                    break;
                case 'expression':
                    warning("Unexpected expression '{a}'.",
                        nexttoken, nexttoken.value);
                    advance();
                    break;
                default:
                    advance();
                }
            } else {
                if (nexttoken.id === ';' || nexttoken.id === '!'  ||
                        nexttoken.id === '(end)' || nexttoken.id === '}') {
                    return true;
                }
                advance();
            }
        }
    }];

    cssBorderStyle = [
        'none', 'hidden', 'dotted', 'dashed', 'solid', 'double', 'ridge',
        'inset', 'outset'
    ];

    cssAttributeData = {
        background: [
            true, 'background-attachment', 'background-color',
            'background-image', 'background-position', 'background-repeat'
        ],
        'background-attachment': ['scroll', 'fixed'],
        'background-color': ['transparent', cssColor],
        'background-image': ['none', cssUrl],
        'background-position': [
            2, [cssLength, 'top', 'bottom', 'left', 'right', 'center']
        ],
        'background-repeat': [
            'repeat', 'repeat-x', 'repeat-y', 'no-repeat'
        ],
        'border': [true, 'border-color', 'border-style', 'border-width'],
        'border-bottom': [true, 'border-bottom-color', 'border-bottom-style', 'border-bottom-width'],
        'border-bottom-color': cssColor,
        'border-bottom-style': cssBorderStyle,
        'border-bottom-width': cssWidth,
        'border-collapse': ['collapse', 'separate'],
        'border-color': ['transparent', 4, cssColor],
        'border-left': [
            true, 'border-left-color', 'border-left-style', 'border-left-width'
        ],
        'border-left-color': cssColor,
        'border-left-style': cssBorderStyle,
        'border-left-width': cssWidth,
        'border-right': [
            true, 'border-right-color', 'border-right-style', 'border-right-width'
        ],
        'border-right-color': cssColor,
        'border-right-style': cssBorderStyle,
        'border-right-width': cssWidth,
        'border-spacing': [2, cssLength],
        'border-style': [4, cssBorderStyle],
        'border-top': [
            true, 'border-top-color', 'border-top-style', 'border-top-width'
        ],
        'border-top-color': cssColor,
        'border-top-style': cssBorderStyle,
        'border-top-width': cssWidth,
        'border-width': [4, cssWidth],
        bottom: [cssLength, 'auto'],
        'caption-side' : ['bottom', 'left', 'right', 'top'],
        clear: ['both', 'left', 'none', 'right'],
        clip: [cssShape, 'auto'],
        color: cssColor,
        content: [
            'open-quote', 'close-quote', 'no-open-quote', 'no-close-quote',
            cssString, cssUrl, cssCounter, cssAttr
        ],
        'counter-increment': [
            cssName, 'none'
        ],
        'counter-reset': [
            cssName, 'none'
        ],
        cursor: [
            cssUrl, 'auto', 'crosshair', 'default', 'e-resize', 'help', 'move',
            'n-resize', 'ne-resize', 'nw-resize', 'pointer', 's-resize',
            'se-resize', 'sw-resize', 'w-resize', 'text', 'wait'
        ],
        direction: ['ltr', 'rtl'],
        display: [
            'block', 'compact', 'inline', 'inline-block', 'inline-table',
            'list-item', 'marker', 'none', 'run-in', 'table', 'table-caption',
            'table-column', 'table-column-group', 'table-footer-group',
            'table-header-group', 'table-row', 'table-row-group'
        ],
        'empty-cells': ['show', 'hide'],
        'float': ['left', 'none', 'right'],
        font: [
            'caption', 'icon', 'menu', 'message-box', 'small-caption', 'status-bar',
            true, 'font-size', 'font-style', 'font-weight', 'font-family'
        ],
        'font-family': cssCommaList,
        'font-size': [
            'xx-small', 'x-small', 'small', 'medium', 'large', 'x-large',
            'xx-large', 'larger', 'smaller', cssLength
        ],
        'font-size-adjust': ['none', cssNumber],
        'font-stretch': [
            'normal', 'wider', 'narrower', 'ultra-condensed',
            'extra-condensed', 'condensed', 'semi-condensed',
            'semi-expanded', 'expanded', 'extra-expanded'
        ],
        'font-style': [
            'normal', 'italic', 'oblique'
        ],
        'font-variant': [
            'normal', 'small-caps'
        ],
        'font-weight': [
            'normal', 'bold', 'bolder', 'lighter', cssNumber
        ],
        height: [cssLength, 'auto'],
        left: [cssLength, 'auto'],
        'letter-spacing': ['normal', cssLength],
        'line-height': ['normal', cssLineHeight],
        'list-style': [
            true, 'list-style-image', 'list-style-position', 'list-style-type'
        ],
        'list-style-image': ['none', cssUrl],
        'list-style-position': ['inside', 'outside'],
        'list-style-type': [
            'circle', 'disc', 'square', 'decimal', 'decimal-leading-zero',
            'lower-roman', 'upper-roman', 'lower-greek', 'lower-alpha',
            'lower-latin', 'upper-alpha', 'upper-latin', 'hebrew', 'katakana',
            'hiragana-iroha', 'katakana-oroha', 'none'
        ],
        margin: [4, cssMargin],
        'margin-bottom': cssMargin,
        'margin-left': cssMargin,
        'margin-right': cssMargin,
        'margin-top': cssMargin,
        'marker-offset': [cssLength, 'auto'],
        'max-height': [cssLength, 'none'],
        'max-width': [cssLength, 'none'],
        'min-height': cssLength,
        'min-width': cssLength,
        opacity: cssNumber,
        outline: [true, 'outline-color', 'outline-style', 'outline-width'],
        'outline-color': ['invert', cssColor],
        'outline-style': [
            'dashed', 'dotted', 'double', 'groove', 'inset', 'none',
            'outset', 'ridge', 'solid'
        ],
        'outline-width': cssWidth,
        overflow: ['auto', 'hidden', 'scroll', 'visible'],
        padding: [4, cssLength],
        'padding-bottom': cssLength,
        'padding-left': cssLength,
        'padding-right': cssLength,
        'padding-top': cssLength,
        position: ['absolute', 'fixed', 'relative', 'static'],
        quotes: [8, cssString],
        right: [cssLength, 'auto'],
        'table-layout': ['auto', 'fixed'],
        'text-align': ['center', 'justify', 'left', 'right'],
        'text-decoration': ['none', 'underline', 'overline', 'line-through', 'blink'],
        'text-indent': cssLength,
        'text-shadow': ['none', 4, [cssColor, cssLength]],
        'text-transform': ['capitalize', 'uppercase', 'lowercase', 'none'],
        top: [cssLength, 'auto'],
        'unicode-bidi': ['normal', 'embed', 'bidi-override'],
        'vertical-align': [
            'baseline', 'bottom', 'sub', 'super', 'top', 'text-top', 'middle',
            'text-bottom', cssLength
        ],
        visibility: ['visible', 'hidden', 'collapse'],
        'white-space': ['normal', 'pre', 'nowrap'],
        width: [cssLength, 'auto'],
        'word-spacing': ['normal', cssLength],
        'z-index': ['auto', cssNumber]
    };

    function styleAttribute() {
        var v;
        while (nexttoken.id === '*' || nexttoken.id === '#' || nexttoken.value === '_') {
            if (!option.css) {
                warning("Unexpected '{a}'.", nexttoken, nexttoken.value);
            }
            advance();
        }
        if (nexttoken.id === '-') {
            if (!option.css) {
                warning("Unexpected '{a}'.", nexttoken, nexttoken.value);
            }
            advance('-');
            if (!nexttoken.identifier) {
                warning("Expected a non-standard style attribute and instead saw '{a}'.",
                    nexttoken, nexttoken.value);
            }
            advance();
            return cssAny;
        } else {
            if (!nexttoken.identifier) {
                warning("Excepted a style attribute, and instead saw '{a}'.",
                    nexttoken, nexttoken.value);
            } else {
                if (cssAttributeData.hasOwnProperty(nexttoken.value)) {
                    v = cssAttributeData[nexttoken.value];
                } else {
                    v = cssAny;
                    if (!option.css) {
                        warning("Unrecognized style attribute '{a}'.",
                                nexttoken, nexttoken.value);
                    }
                }
            }
            advance();
            return v;
        }
    }

    function styleValue(v) {
        var i = 0,
            n,
            once,
            match,
            round,
            start = 0,
            vi;
        switch (typeof v) {
        case 'function':
            return v();
        case 'string':
            if (nexttoken.identifier && nexttoken.value === v) {
                advance();
                return true;
            }
            return false;
        }
        for (;;) {
            if (i >= v.length) {
                return false;
            }
            vi = v[i];
            i += 1;
            if (vi === true) {
                break;
            } else if (typeof vi === 'number') {
                n = vi;
                vi = v[i];
                i += 1;
            } else {
                n = 1;
            }
            match = false;
            while (n > 0) {
                if (styleValue(vi)) {
                    match = true;
                    n -= 1;
                } else {
                    break;
                }
            }
            if (match) {
                return true;
            }
        }
        start = i;
        once = [];
        for (;;) {
            round = false;
            for (i = start; i < v.length; i += 1) {
                if (!once[i]) {
                    if (styleValue(cssAttributeData[v[i]])) {
                        match = true;
                        round = true;
                        once[i] = true;
                        break;
                    }
                }
            }
            if (!round) {
                return match;
            }
        }
    }

    function substyle() {
        var v;
        for (;;) {
            if (nexttoken.id === '}' || nexttoken.id === '(end)' ||
                    xquote && nexttoken.id === xquote) {
                return;
            }
            while (nexttoken.id === ';') {
                warning("Misplaced ';'.");
                advance(';');
            }
            v = styleAttribute();
            advance(':');
            if (nexttoken.identifier && nexttoken.value === 'inherit') {
                advance();
            } else {
                styleValue(v);
            }
            while (nexttoken.id !== ';' && nexttoken.id !== '!' &&
                    nexttoken.id !== '}' && nexttoken.id !== '(end)' &&
                    nexttoken.id !== xquote) {
                warning("Unexpected token '{a}'.", nexttoken, nexttoken.value);
                advance();
            }
            if (nexttoken.id === '!') {
                advance('!');
                adjacent();
                if (nexttoken.identifier && nexttoken.value === 'important') {
                    advance();
                } else {
                    warning("Expected '{a}' and instead saw '{b}'.",
                        nexttoken, 'important', nexttoken.value);
                }
            }
            if (nexttoken.id === '}' || nexttoken.id === xquote) {
                warning("Missing '{a}'.", nexttoken, ';');
            } else {
                advance(';');
            }
        }
    }

    function stylePattern() {
        var name;
        if (nexttoken.id === '{') {
            warning("Expected a style pattern, and instead saw '{a}'.", nexttoken,
                nexttoken.id);
        } else if (nexttoken.id === '@') {
            advance('@');
            name = nexttoken.value;
            if (nexttoken.identifier && atrule[name] === true) {
                advance();
                return name;
            }
            warning("Expected an at-rule, and instead saw @{a}.", nexttoken, name);
        }
        for (;;) {
            if (nexttoken.identifier) {
                if (!htmltag.hasOwnProperty(nexttoken.value)) {
                    warning("Expected a tagName, and instead saw {a}.",
                        nexttoken, nexttoken.value);
                }
                advance();
            } else {
                switch (nexttoken.id) {
                case '>':
                case '+':
                    advance();
                    if (!nexttoken.identifier ||
                            !htmltag.hasOwnProperty(nexttoken.value)) {
                        warning("Expected a tagName, and instead saw {a}.",
                            nexttoken, nexttoken.value);
                    }
                    advance();
                    break;
                case ':':
                    advance(':');
                    if (pseudorule[nexttoken.value] !== true) {
                        warning("Expected a pseudo, and instead saw :{a}.",
                            nexttoken, nexttoken.value);
                    }
                    advance();
                    if (nexttoken.value === 'lang') {
                        advance('(');
                        if (!nexttoken.identifier) {
                            warning("Expected a lang code, and instead saw :{a}.",
                                nexttoken, nexttoken.value);
                        }
                        advance(')');
                    }
                    break;
                case '#':
                    advance('#');
                    if (!nexttoken.identifier) {
                        warning("Expected an id, and instead saw #{a}.",
                            nexttoken, nexttoken.value);
                    }
                    advance();
                    break;
                case '*':
                    advance('*');
                    break;
                case '.':
                    advance('.');
                    if (!nexttoken.identifier) {
                        warning("Expected a class, and instead saw #.{a}.",
                            nexttoken, nexttoken.value);
                    }
                    advance();
                    break;
                case '[':
                    advance('[');
                    if (!nexttoken.identifier) {
                        warning("Expected an attribute, and instead saw [{a}].",
                            nexttoken, nexttoken.value);
                    }
                    advance();
                    if (nexttoken.id === '=' || nexttoken.id === '~=' ||
                            nexttoken.id === '|=') {
                        advance();
                        if (nexttoken.type !== '(string)') {
                            warning("Expected a string, and instead saw {a}.",
                                nexttoken, nexttoken.value);
                        }
                        advance();
                    }
                    advance(']');
                    break;
                default:
                    error("Expected a CSS selector, and instead saw {a}.",
                        nexttoken, nexttoken.value);
                }
            }
            if (nexttoken.id === '</' || nexttoken.id === '{' ||
                    nexttoken.id === '(end)') {
                return '';
            }
            if (nexttoken.id === ',') {
                advance(',');
            }
        }
    }

    function styles() {
        while (nexttoken.id !== '</' && nexttoken.id !== '(end)') {
            stylePattern();
            xmode = 'styleproperty';
            if (nexttoken.id === ';') {
                advance(';');
            } else {
                advance('{');
                substyle();
                xmode = 'style';
                advance('}');
            }
        }
    }


// HTML parsing.

    function doBegin(n) {
        if (n !== 'html' && !option.fragment) {
            if (n === 'div' && option.adsafe) {
                error("ADSAFE: Use the fragment option.");
            } else {
                error("Expected '{a}' and instead saw '{b}'.",
                    token, 'html', n);
            }
        }
        if (option.adsafe) {
            if (n === 'html') {
                error("Currently, ADsafe does not operate on whole HTML documents. It operates on <div> fragments and .js files.", token);
            }
            if (option.fragment) {
                if (n !== 'div') {
                    error("ADsafe violation: Wrap the widget in a div.", token);
                }
            } else {
                error("Use the fragment option.", token);
            }
        }
        option.browser = true;
        assume();
    }

    function doAttribute(n, a, v) {
        var u, x;
        if (a === 'id') {
            u = typeof v === 'string' ? v.toUpperCase() : '';
            if (ids[u] === true) {
                warning("Duplicate id='{a}'.", nexttoken, v);
            }
            if (option.adsafe) {
                if (adsafe_id) {
                    if (v.slice(0, adsafe_id.length) !== adsafe_id) {
                        warning("ADsafe violation: An id must have a '{a}' prefix",
                                nexttoken, adsafe_id);
                    } else if (!/^[A-Z]+_[A-Z]+$/.test(v)) {
                        warning("ADSAFE violation: bad id.");
                    }
                } else {
                    adsafe_id = v;
                    if (!/^[A-Z]+_$/.test(v)) {
                        warning("ADSAFE violation: bad id.");
                    }
                }
            }
            x = v.search(dx);
            if (x >= 0) {
                warning("Unexpected character '{a}' in {b}.", token, v.charAt(x), a);
            }
            ids[u] = true;
        } else if (a === 'class' || a === 'type' || a === 'name') {
            x = v.search(qx);
            if (x >= 0) {
                warning("Unexpected character '{a}' in {b}.", token, v.charAt(x), a);
            }
            ids[u] = true;
        } else if (a === 'href' || a === 'background' ||
                a === 'content' || a === 'data' ||
                a.indexOf('src') >= 0 || a.indexOf('url') >= 0) {
            if (option.safe && ux.test(v)) {
                error("ADsafe URL violation.");
            }
            urls.push(v);
        } else if (a === 'for') {
            if (option.adsafe) {
                if (adsafe_id) {
                    if (v.slice(0, adsafe_id.length) !== adsafe_id) {
                        warning("ADsafe violation: An id must have a '{a}' prefix",
                                nexttoken, adsafe_id);
                    } else if (!/^[A-Z]+_[A-Z]+$/.test(v)) {
                        warning("ADSAFE violation: bad id.");
                    }
                } else {
                    warning("ADSAFE violation: bad id.");
                }
            }
        } else if (a === 'name') {
            if (option.adsafe && v.indexOf('_') >= 0) {
                warning("ADsafe name violation.");
            }
        }
    }

    function doTag(n, a) {
        var i, t = htmltag[n], x;
        src = false;
        if (!t) {
            error("Unrecognized tag '<{a}>'.",
                    nexttoken,
                    n === n.toLowerCase() ? n :
                        n + ' (capitalization error)');
        }
        if (stack.length > 0) {
            if (n === 'html') {
                error("Too many <html> tags.", token);
            }
            x = t.parent;
            if (x) {
                if (x.indexOf(' ' + stack[stack.length - 1].name + ' ') < 0) {
                    error("A '<{a}>' must be within '<{b}>'.",
                            token, n, x);
                }
            } else if (!option.adsafe && !option.fragment) {
                i = stack.length;
                do {
                    if (i <= 0) {
                        error("A '<{a}>' must be within '<{b}>'.",
                                token, n, 'body');
                    }
                    i -= 1;
                } while (stack[i].name !== 'body');
            }
        }
        switch (n) {
        case 'div':
            if (option.adsafe && stack.length === 1 && !adsafe_id) {
                warning("ADSAFE violation: missing ID_.");
            }
            break;
        case 'script':
            xmode = 'script';
            advance('>');
            indent = nexttoken.from;
            if (a.lang) {
                warning("lang is deprecated.", token);
            }
            if (option.adsafe && stack.length !== 1) {
                warning("ADsafe script placement violation.", token);
            }
            if (a.src) {
                if (option.adsafe && (!adsafe_may || !approved[a.src])) {
                    warning("ADsafe unapproved script source.", token);
                }
                if (a.type) {
                    warning("type is unnecessary.", token);
                }
            } else {
                if (adsafe_went) {
                    error("ADsafe script violation.", token);
                }
                statements('script');
            }
            xmode = 'html';
            advance('</');
            if (!nexttoken.identifier && nexttoken.value !== 'script') {
                warning("Expected '{a}' and instead saw '{b}'.",
                        nexttoken, 'script', nexttoken.value);
            }
            advance();
            xmode = 'outer';
            break;
        case 'style':
            xmode = 'style';
            advance('>');
            styles();
            xmode = 'html';
            advance('</');
            if (!nexttoken.identifier && nexttoken.value !== 'style') {
                warning("Expected '{a}' and instead saw '{b}'.",
                        nexttoken, 'style', nexttoken.value);
            }
            advance();
            xmode = 'outer';
            break;
        case 'input':
            switch (a.type) {
            case 'radio':
            case 'checkbox':
            case 'text':
            case 'button':
            case 'file':
            case 'reset':
            case 'submit':
            case 'password':
            case 'file':
            case 'hidden':
            case 'image':
                break;
            default:
                warning("Bad input type.");
            }
            if (option.adsafe && a.autocomplete !== 'off') {
                warning("ADsafe autocomplete violation.");
            }
            break;
        case 'applet':
        case 'body':
        case 'embed':
        case 'frame':
        case 'frameset':
        case 'head':
        case 'iframe':
        case 'img':
        case 'noembed':
        case 'noframes':
        case 'object':
        case 'param':
            if (option.adsafe) {
                warning("ADsafe violation: Disallowed tag: " + n);
            }
            break;
        }
    }


    function closetag(n) {
        return '</' + n + '>';
    }

    function html() {
        var a, attributes, e, n, q, t, v, w = option.white, wmode;
        xmode = 'html';
        xquote = '';
        stack = null;
        for (;;) {
            switch (nexttoken.value) {
            case '<':
                xmode = 'html';
                advance('<');
                attributes = {};
                t = nexttoken;
                if (!t.identifier) {
                    warning("Bad identifier {a}.", t, t.value);
                }
                n = t.value;
                if (option.cap) {
                    n = n.toLowerCase();
                }
                t.name = n;
                advance();
                if (!stack) {
                    stack = [];
                    doBegin(n);
                }
                v = htmltag[n];
                if (typeof v !== 'object') {
                    error("Unrecognized tag '<{a}>'.", t, n);
                }
                e = v.empty;
                t.type = n;
                for (;;) {
                    if (nexttoken.id === '/') {
                        advance('/');
                        if (nexttoken.id !== '>') {
                            warning("Expected '{a}' and instead saw '{b}'.",
                                    nexttoken, '>', nexttoken.value);
                        }
                        break;
                    }
                    if (nexttoken.id && nexttoken.id.substr(0, 1) === '>') {
                        break;
                    }
                    if (!nexttoken.identifier) {
                        if (nexttoken.id === '(end)' || nexttoken.id === '(error)') {
                            error("Missing '>'.", nexttoken);
                        }
                        warning("Bad identifier.");
                    }
                    option.white = true;
                    nonadjacent(token, nexttoken);
                    a = nexttoken.value;
                    option.white = w;
                    advance();
                    if (!option.cap && a !== a.toLowerCase()) {
                        warning("Attribute '{a}' not all lower case.", nexttoken, a);
                    }
                    a = a.toLowerCase();
                    xquote = '';
                    if (attributes.hasOwnProperty(a)) {
                        warning("Attribute '{a}' repeated.", nexttoken, a);
                    }
                    if (a.slice(0, 2) === 'on') {
                        if (!option.on) {
                            warning("Avoid HTML event handlers.");
                        }
                        xmode = 'scriptstring';
                        advance('=');
                        q = nexttoken.id;
                        if (q !== '"' && q !== "'") {
                            error("Missing quote.");
                        }
                        xquote = q;
                        wmode = option.white;
                        option.white = false;
                        advance(q);
                        statements('on');
                        option.white = wmode;
                        if (nexttoken.id !== q) {
                            error("Missing close quote on script attribute.");
                        }
                        xmode = 'html';
                        xquote = '';
                        advance(q);
                        v = false;
                    } else if (a === 'style') {
                        xmode = 'scriptstring';
                        advance('=');
                        q = nexttoken.id;
                        if (q !== '"' && q !== "'") {
                            error("Missing quote.");
                        }
                        xmode = 'styleproperty';
                        xquote = q;
                        advance(q);
                        substyle();
                        xmode = 'html';
                        xquote = '';
                        advance(q);
                        v = false;
                    } else {
                        if (nexttoken.id === '=') {
                            advance('=');
                            v = nexttoken.value;
                            if (!nexttoken.identifier &&
                                    nexttoken.id !== '"' &&
                                    nexttoken.id !== '\'' &&
                                    nexttoken.type !== '(string)' &&
                                    nexttoken.type !== '(number)' &&
                                    nexttoken.type !== '(color)') {
                                warning("Expected an attribute value and instead saw '{a}'.", token, a);
                            }
                            advance();
                        } else {
                            v = true;
                        }
                    }
                    attributes[a] = v;
                    doAttribute(n, a, v);
                }
                doTag(n, attributes);
                if (!e) {
                    stack.push(t);
                }
                xmode = 'outer';
                advance('>');
                break;
            case '</':
                xmode = 'html';
                advance('</');
                if (!nexttoken.identifier) {
                    warning("Bad identifier.");
                }
                n = nexttoken.value;
                if (option.cap) {
                    n = n.toLowerCase();
                }
                advance();
                if (!stack) {
                    error("Unexpected '{a}'.", nexttoken, closetag(n));
                }
                t = stack.pop();
                if (!t) {
                    error("Unexpected '{a}'.", nexttoken, closetag(n));
                }
                if (t.name !== n) {
                    error("Expected '{a}' and instead saw '{b}'.",
                            nexttoken, closetag(t.name), closetag(n));
                }
                if (nexttoken.id !== '>') {
                    error("Missing '{a}'.", nexttoken, '>');
                }
                xmode = 'outer';
                advance('>');
                break;
            case '<!':
                if (option.safe) {
                    warning("ADsafe HTML violation.");
                }
                xmode = 'html';
                for (;;) {
                    advance();
                    if (nexttoken.id === '>' || nexttoken.id === '(end)') {
                        break;
                    }
                    if (nexttoken.id === '--') {
                        warning("Unexpected --.");
                    }
                }
                xmode = 'outer';
                advance('>');
                break;
            case '<!--':
                xmode = 'html';
                if (option.safe) {
                    warning("ADsafe HTML violation.");
                }
                for (;;) {
                    advance();
                    if (nexttoken.id === '(end)') {
                        error("Missing '-->'.");
                    }
                    if (nexttoken.id === '<!' || nexttoken.id === '<!--') {
                        error("Unexpected '<!' in HTML comment.");
                    }
                    if (nexttoken.id === '--') {
                        advance('--');
                        break;
                    }
                }
                abut();
                xmode = 'outer';
                advance('>');
                break;
            case '(end)':
                return;
            default:
                if (nexttoken.id === '(end)') {
                    error("Missing '{a}'.", nexttoken,
                            '</' + stack[stack.length - 1].value + '>');
                } else {
                    advance();
                }
            }
            if (stack && stack.length === 0 && (option.adsafe ||
                    !option.fragment || nexttoken.id === '(end)')) {
                break;
            }
        }
        if (nexttoken.id !== '(end)') {
            error("Unexpected material after the end.");
        }
    }


// Build the syntax table by declaring the syntactic elements of the language.

    type('(number)', idValue);
    type('(string)', idValue);

    syntax['(identifier)'] = {
        type: '(identifier)',
        lbp: 0,
        identifier: true,
        nud: function () {
            var v = this.value,
                s = scope[v];

// The name is in scope and defined in the current function.

            if (s && (s === funct || s === funct['(global)'])) {

//      If we are not also in the global scope, change 'unused' to 'var',
//      and reject labels.

                if (!funct['(global)']) {
                    switch (funct[v]) {
                    case 'unused':
                        funct[v] = 'var';
                        break;
                    case 'label':
                        warning("'{a}' is a statement label.", token, v);
                        break;
                    }
                }

// The name is not defined in the function.  If we are in the global scope,
// then we have an undefined variable.

            } else if (funct['(global)']) {
                if (option.undef) {
                    warning("'{a}' is undefined.", token, v);
                }
                note_implied(token);

// If the name is already defined in the current
// function, but not as outer, then there is a scope error.

            } else {
                switch (funct[v]) {
                case 'closure':
                case 'function':
                case 'var':
                case 'unused':
                case 'const':
                    warning("'{a}' used out of scope.", token, v);
                    break;
                case 'label':
                    warning("'{a}' is a statement label.", token, v);
                    break;
                case 'outer':
                case true:
                    break;
                default:

// If the name is defined in an outer function, make an outer entry, and if
// it was unused, make it var.

                    if (s === true) {
                        funct[v] = true;
                    } else if (typeof s !== 'object') {
                        if (option.undef) {
                            warning("'{a}' is undefined.", token, v);
                        } else {
                            funct[v] = true;
                        }
                        note_implied(token);
                    } else {
                        switch (s[v]) {
                        case 'function':
                        case 'var':
                        case 'unused':
						case 'const':
                            s[v] = 'closure';
                            funct[v] = 'outer';
                            break;
                        case 'closure':
                        case 'parameter':
                            funct[v] = 'outer';
                            break;
                        case 'label':
                            warning("'{a}' is a statement label.", token, v);
                        }
                    }
                }
            }
            return this;
        },
        led: function () {
            error("Expected an operator and instead saw '{a}'.",
                    nexttoken, nexttoken.value);
        }
    };

    type('(regexp)', function () {
        return this;
    });

    delim('(endline)');
    delim('(begin)');
    delim('(end)').reach = true;
    delim('</').reach = true;
    delim('<!');
    delim('<!--');
    delim('-->');
    delim('(error)').reach = true;
    delim('}').reach = true;
    delim(')');
    delim(']');
    delim('"').reach = true;
    delim("'").reach = true;
    delim(';');
    delim(':').reach = true;
    delim(',');
    delim('#');
    delim('@');
    reserve('else');
    reserve('case').reach = true;
    reserve('catch');
    reserve('default').reach = true;
    reserve('finally');
    reservevar('arguments');
    reservevar('eval');
    reservevar('false');
    reservevar('Infinity');
    reservevar('NaN');
    reservevar('null');
    reservevar('this');
    reservevar('true');
    reservevar('undefined');
    assignop('=', 'assign', 20);
    assignop('+=', 'assignadd', 20);
    assignop('-=', 'assignsub', 20);
    assignop('*=', 'assignmult', 20);
    assignop('/=', 'assigndiv', 20).nud = function () {
        error("A regular expression literal can be confused with '/='.");
    };
    assignop('%=', 'assignmod', 20);
    bitwiseassignop('&=', 'assignbitand', 20);
    bitwiseassignop('|=', 'assignbitor', 20);
    bitwiseassignop('^=', 'assignbitxor', 20);
    bitwiseassignop('<<=', 'assignshiftleft', 20);
    bitwiseassignop('>>=', 'assignshiftright', 20);
    bitwiseassignop('>>>=', 'assignshiftrightunsigned', 20);
    infix('?', function (left) {
        this.left = left;
        this.right = parse(10);
        advance(':');
        this['else'] = parse(10);
        return this;
    }, 30);

    infix('||', 'or', 40);
    infix('&&', 'and', 50);
    bitwise('|', 'bitor', 70);
    bitwise('^', 'bitxor', 80);
    bitwise('&', 'bitand', 90);
    relation('==', function (left, right) {
        if (option.eqeqeq) {
            warning("Expected '{a}' and instead saw '{b}'.",
                    this, '===', '==');
        } else if (isPoorRelation(left)) {
            warning("Use '{a}' to compare with '{b}'.",
                this, '===', left.value);
        } else if (isPoorRelation(right)) {
            warning("Use '{a}' to compare with '{b}'.",
                this, '===', right.value);
        }
        return this;
    });
    relation('===');
    relation('!=', function (left, right) {
        if (option.eqeqeq) {
            warning("Expected '{a}' and instead saw '{b}'.",
                    this, '!==', '!=');
        } else if (isPoorRelation(left)) {
            warning("Use '{a}' to compare with '{b}'.",
                    this, '!==', left.value);
        } else if (isPoorRelation(right)) {
            warning("Use '{a}' to compare with '{b}'.",
                    this, '!==', right.value);
        }
        return this;
    });
    relation('!==');
    relation('<');
    relation('>');
    relation('<=');
    relation('>=');
    bitwise('<<', 'shiftleft', 120);
    bitwise('>>', 'shiftright', 120);
    bitwise('>>>', 'shiftrightunsigned', 120);
    infix('in', 'in', 120);
    infix('instanceof', 'instanceof', 120);
    infix('+', function (left) {
        nonadjacent(prevtoken, token);
        nonadjacent(token, nexttoken);
        var right = parse(130);
        if (left && right && left.id === '(string)' && right.id === '(string)') {
            left.value += right.value;
            left.character = right.character;
            if (jx.test(left.value)) {
                warning("JavaScript URL.", left);
            }
            return left;
        }
        this.left = left;
        this.right = right;
        return this;
    }, 130);
    prefix('+', 'num');
    infix('-', 'sub', 130);
    prefix('-', 'neg');
    infix('*', 'mult', 140);
    infix('/', 'div', 140);
    infix('%', 'mod', 140);

    suffix('++', 'postinc');
    prefix('++', 'preinc');
    syntax['++'].exps = true;

    suffix('--', 'postdec');
    prefix('--', 'predec');
    syntax['--'].exps = true;
    prefix('delete', function () {
        var p = parse(0);
        if (p.id !== '.' && p.id !== '[') {
            warning("Expected '{a}' and instead saw '{b}'.",
                    nexttoken, '.', nexttoken.value);
        }
    }).exps = true;


    prefix('~', function () {
        if (option.bitwise) {
            warning("Unexpected '{a}'.", this, '~');
        }
        parse(150);
        return this;
    });
    prefix('!', 'not');
    prefix('typeof', 'typeof');
    prefix('new', function () {
        var c = parse(155), i;
        if (c && c.id !== 'function') {
            if (c.identifier) {
                c['new'] = true;
                switch (c.value) {
                case 'Object':
                    if(!option.noliteral) {
                        warning("Use the object literal notation {}.", token);
                    }
                    break;
                case 'Array':
                    if(!option.noliteral) {
                        warning("Use the array literal notation [].", token);
                    }
                    break;
                case 'Number':
                case 'String':
                case 'Boolean':
                case 'Math':
                    warning("Do not use {a} as a constructor.", token, c.value);
                    break;
                case 'Function':
                    if (!option.evil) {
                        warning("The Function constructor is eval.");
                    }
                    break;
                case 'Date':
                case 'RegExp':
                    break;
                default:
                    if (c.id !== 'function') {
                        i = c.value.substr(0, 1);
                        if (option.newcap && (i < 'A' || i > 'Z')) {
                            warning(
                    "A constructor name should start with an uppercase letter.",
                                token);
                        }
                    }
                }
            } else {
                if (c.id !== '.' && c.id !== '[' && c.id !== '(') {
                    warning("Bad constructor.", token);
                }
            }
        } else {
            warning("Weird construction. Delete 'new'.", this);
        }
        adjacent(token, nexttoken);
        if (nexttoken.id !== '(') {
            warning("Missing '()' invoking a constructor.");
        }
        this.first = c;
        return this;
    });
    syntax['new'].exps = true;

    infix('.', function (left) {
        adjacent(prevtoken, token);
        var t = this, m = identifier();
        if (typeof m === 'string') {
            countMember(m);
        }
        t.left = left;
        t.right = m;
        if (!option.evil && left && left.value === 'document' &&
                (m === 'write' || m === 'writeln')) {
            warning("document.write can be a form of eval.", left);
        }
        if (option.adsafe) {
            if (left && left.value === 'ADSAFE') {
                if (m === 'id' || m === 'lib') {
                    warning("ADsafe violation.", this);
                } else if (m === 'go') {
                    if (xmode !== 'script') {
                        warning("ADsafe violation.", this);
                    } else if (adsafe_went || nexttoken.id !== '(' ||
                            peek(0).id !== '(string)' ||
                            peek(0).value !== adsafe_id ||
                            peek(1).id !== ',') {
                        error("ADsafe violation: go.", this);
                    }
                    adsafe_went = true;
                    adsafe_may = false;
                }
            }
        }
        if (option.safe) {
            for (;;) {
                if (banned[m] === true) {
                    warning("ADsafe restricted word '{a}'.", token, m);
                }
                if (predefined[left.value] !== true ||
                        nexttoken.id === '(') {
                    break;
                }
                if (standard_member[m] === true) {
                    if (nexttoken.id === '.') {
                        warning("ADsafe violation.", this);
                    }
                    break;
                }
                if (nexttoken.id !== '.') {
                    warning("ADsafe violation.", this);
                    break;
                }
                advance('.');
                token.left = t;
                token.right = m;
                t = token;
                m = identifier();
                if (typeof m === 'string') {
                    countMember(m);
                }
            }
        }
        return t;
    }, 160);

    infix('(', function (left) {
        adjacent(prevtoken, token);
        nospace();
        var n = 0,
            p = [];
        if (left) {
            if (left.type === '(identifier)') {
                if (left.value.match(/^[A-Z]([A-Z0-9_$]*[a-z][A-Za-z0-9_$]*)?$/)) {
                    if (left.value !== 'Number' && left.value !== 'String' &&
                            left.value !== 'Boolean' && left.value !== 'Date') {
                        if (left.value === 'Math') {
                            warning("Math is not a function.", left);
                        } else if (option.newcap) {
                            warning("Missing 'new' prefix when invoking a constructor.",
                                left);
                        }
                    }
                }
            } else if (left.id === '.') {
                if (option.safe && left.left.value === 'Math' &&
                        left.right === 'random') {
                    warning("ADsafe violation.", left);
                }
            }
        }
        if (nexttoken.id !== ')') {
            for (;;) {
                p[p.length] = parse(10);
                n += 1;
                if (nexttoken.id !== ',') {
                    break;
                }
                advance(',');
                nonadjacent(token, nexttoken);
            }
        }
        advance(')');
        nospace(prevtoken, token);
        if (typeof left === 'object') {
            if(!option.noradix) {
                if (left.value === 'parseInt' && n === 1) {
                    warning("Missing radix parameter.", left);
                }
            }
            if (!option.evil) {
                if (left.value === 'eval' || left.value === 'Function' ||
                        left.value === 'execScript') {
                    warning("eval is evil.", left);
                } else if (p[0] && p[0].id === '(string)' &&
                       (left.value === 'setTimeout' ||
                        left.value === 'setInterval')) {
                    warning(
    "Implied eval is evil. Pass a function instead of a string.", left);
                }
            }
			if (left.value === 'load') {
				// Handle load() stuff here!
				
			}
            if (!left.identifier && left.id !== '.' && left.id !== '[' &&
                    left.id !== '(' && left.id !== '&&' && left.id !== '||' &&
                    left.id !== '?') {
                warning("Bad invocation.", left);
            }

        }
        this.left = left;
        return this;
    }, 155).exps = true;

    prefix('(', function () {
        nospace();
        var v = parse(0);
        advance(')', this);
        nospace(prevtoken, token);
        return v;
    });

    infix('[', function (left) {
        nospace();
        var e = parse(0), s;
        if (e && e.type === '(string)') {
            if (option.safe && banned[e.value] === true) {
                warning("ADsafe restricted word '{a}'.", this, e.value);
            }
            countMember(e.value);
            if (!option.sub && ix.test(e.value)) {
                s = syntax[e.value];
                if (!s || !s.reserved) {
                    warning("['{a}'] is better written in dot notation.",
                            e, e.value);
                }
            }
        } else if (!e || (e.type !== '(number)' &&
                (e.id !== '+' || e.arity !== 'unary'))) {
            if (option.safe) {
                warning('ADsafe subscripting.');
            }
        }
        advance(']', this);
        nospace(prevtoken, token);
        this.left = left;
        this.right = e;
        return this;
    }, 160);

    prefix('[', function () {
        if (nexttoken.id === ']') {
            advance(']');
            return;
        }
        var b = token.line !== nexttoken.line;
        if (b) {
            indent += option.indent;
            if (nexttoken.from === indent + option.indent) {
                indent += option.indent;
            }
        }
        for (;;) {
            if (b && token.line !== nexttoken.line) {
                indentation();
            }
            parse(10);
            if (nexttoken.id === ',') {
                adjacent(token, nexttoken);
                advance(',');
                if (nexttoken.id === ',') {
                    warning("Extra comma.", token);
                } else if (nexttoken.id === ']') {
                    if(!option.noextracomma) {
                        warning("Extra comma.", token);
                    }
                    break;
                }
                nonadjacent(token, nexttoken);
            } else {
                if (b) {
                    indent -= option.indent;
                    indentation();
                }
                break;
            }
        }
        advance(']', this);
        return;
    }, 160);

    (function (x) {
        x.nud = function () {
            var b, i, s, seen = {};
            b = token.line !== nexttoken.line;
            if (b) {
                indent += option.indent;
                if (nexttoken.from === indent + option.indent) {
                    indent += option.indent;
                }
            }
            for (;;) {
                if (nexttoken.id === '}') {
                    break;
                }
                if (b) {
                    indentation();
                }
                i = optionalidentifier(true);
                if (!i) {
                    if (nexttoken.id === '(string)') {
                        i = nexttoken.value;
                        if (ix.test(i)) {
                            s = syntax[i];
                        }
                        advance();
                    } else if (nexttoken.id === '(number)') {
                        i = nexttoken.value.toString();
                        advance();
                    } else {
                        error("Expected '{a}' and instead saw '{b}'.",
                                nexttoken, '}', nexttoken.value);
                    }
                }
                if (seen[i] === true) {
                    warning("Duplicate member '{a}'.", nexttoken, i);
                }
                seen[i] = true;
                countMember(i);
                advance(':');
                nonadjacent(token, nexttoken);
                parse(10);
                if (nexttoken.id === ',') {
                    adjacent(token, nexttoken);
                    advance(',');
                    if (nexttoken.id === ',') {
                        warning("Extra comma.", token);
                    } else if(nexttoken.id === '}') {
                        if(!option.noextracomma) {
                            warning("Extra comma.", token);
                        }
                    }
                    nonadjacent(token, nexttoken);
                } else {
                    break;
                }
            }
            if (b) {
                indent -= option.indent;
                indentation();
            }
            advance('}', this);
            return;
        };
        x.fud = function () {
            error("Expected to see a statement and instead saw a block.", token);
        };
    })(delim('{'));


    function varstatement(prefix) {

// JavaScript does not have block scope. It only has function scope. So,
// declaring a variable in a block can have unexpected consequences.

        if (funct['(onevar)'] && option.onevar) {
            warning("Too many var statements.");
        } else if (!funct['(global)']) {
            funct['(onevar)'] = true;
        }
        for (;;) {
            nonadjacent(token, nexttoken);
            addlabel(identifier(), 'unused');
            if (prefix) {
                return;
            }
            if (nexttoken.id === '=') {
                nonadjacent(token, nexttoken);
                advance('=');
                nonadjacent(token, nexttoken);
                if (peek(0).id === '=') {
                    error("Variable {a} was not declared correctly.",
                            nexttoken, nexttoken.value);
                }
                parse(20);
            }
            if (nexttoken.id !== ',') {
                return;
            }
            adjacent(token, nexttoken);
            advance(',');
            nonadjacent(token, nexttoken);
        }
    }


    stmt('var', varstatement);

    stmt('const', varstatement);

    stmt('new', function () {
        error("'new' should not be used as a statement.");
    });


    function functionparams() {
        var i, t = nexttoken, p = [];
        advance('(');
        nospace();
        if (nexttoken.id === ')') {
            advance(')');
            nospace(prevtoken, token);
            return;
        }
        for (;;) {
            i = identifier();
            p.push(i);
            addlabel(i, 'parameter');
            if (nexttoken.id === ',') {
                advance(',');
                nonadjacent(token, nexttoken);
            } else {
                advance(')', t);
                nospace(prevtoken, token);
                return p.join(', ');
            }
        }
    }

    function doFunction(i) {
        var s = scope;
        scope = Object.create(s);
        funct = {
            '(name)'    : i || '"' + anonname + '"',
            '(line)'    : nexttoken.line + 1,
            '(context)' : funct,
            '(breakage)': 0,
            '(loopage)' : 0,
            '(scope)'   : scope
        };
        functions.push(funct);
        if (i) {
            addlabel(i, 'function');
        }
        funct['(params)'] = functionparams();

        block(false);
        scope = s;
        funct = funct['(context)'];
    }


    blockstmt('function', function () {
        if (inblock) {
            warning(
"Function statements cannot be placed in blocks. Use a function expression or move the statement to the top of the outer function.", token);

        }
        var i = identifier();
        adjacent(token, nexttoken);
        addlabel(i, 'unused');
        doFunction(i);
        if (nexttoken.id === '(' && nexttoken.line === token.line) {
            error(
"Function statements are not invocable. Wrap the function expression in parens.");
        }
    });

    prefix('function', function () {
        var i = optionalidentifier();
        if (i) {
            adjacent(token, nexttoken);
        } else {
            nonadjacent(token, nexttoken);
        }
        doFunction(i);
        if (funct['(loopage)'] && nexttoken.id !== '(') {
            warning("Be careful when making functions within a loop. Consider putting the function in a closure.");
        }
        return this;
    });

    blockstmt('if', function () {
        var t = nexttoken;
        advance('(');
        nonadjacent(this, t);
        nospace();
        parse(20);
        if (nexttoken.id === '=') {
            warning("Expected a conditional expression and instead saw an assignment.");
            advance('=');
            parse(20);
        }
        advance(')', t);
        nospace(prevtoken, token);
        block(true);
        if (nexttoken.id === 'else') {
            nonadjacent(token, nexttoken);
            advance('else');
            if (nexttoken.id === 'if' || nexttoken.id === 'switch') {
                statement(true);
            } else {
                block(true);
            }
        }
        return this;
    });

    blockstmt('try', function () {
        var b, e, s;
        if (option.adsafe) {
            warning("ADsafe try violation.", this);
        }
        block(false);
        if (nexttoken.id === 'catch') {
            advance('catch');
            nonadjacent(token, nexttoken);
            advance('(');
            s = scope;
            scope = Object.create(s);
            e = nexttoken.value;
            if (nexttoken.type !== '(identifier)') {
                warning("Expected an identifier and instead saw '{a}'.",
                    nexttoken, e);
            } else {
                addlabel(e, 'unused');
            }
            advance();
            advance(')');
            block(false);
            b = true;
            scope = s;
        }
        if (nexttoken.id === 'finally') {
            advance('finally');
            block(false);
            return;
        } else if (!b) {
            error("Expected '{a}' and instead saw '{b}'.",
                    nexttoken, 'catch', nexttoken.value);
        }
    });

    blockstmt('while', function () {
        var t = nexttoken;
        funct['(breakage)'] += 1;
        funct['(loopage)'] += 1;
        advance('(');
        nonadjacent(this, t);
        nospace();
        parse(20);
        if (nexttoken.id === '=') {
            warning("Expected a conditional expression and instead saw an assignment.");
            advance('=');
            parse(20);
        }
        advance(')', t);
        nospace(prevtoken, token);
        block(true);
        funct['(breakage)'] -= 1;
        funct['(loopage)'] -= 1;
    }).labelled = true;

    reserve('with');

    blockstmt('switch', function () {
        var t = nexttoken,
            g = false;
        funct['(breakage)'] += 1;
        advance('(');
        nonadjacent(this, t);
        nospace();
        this.condition = parse(20);
        advance(')', t);
        nospace(prevtoken, token);
        nonadjacent(token, nexttoken);
        t = nexttoken;
        advance('{');
        nonadjacent(token, nexttoken);
        indent += option.indent;
        this.cases = [];
        for (;;) {
            switch (nexttoken.id) {
            case 'case':
                switch (funct['(verb)']) {
                case 'break':
                case 'case':
                case 'continue':
                case 'return':
                case 'switch':
                case 'throw':
                    break;
                default:
                    warning(
                        "Expected a 'break' statement before 'case'.",
                        token);
                }
                indentation(-option.indent);
                advance('case');
                this.cases.push(parse(20));
                g = true;
                advance(':');
                funct['(verb)'] = 'case';
                break;
            case 'default':
                switch (funct['(verb)']) {
                case 'break':
                case 'continue':
                case 'return':
                case 'throw':
                    break;
                default:
                    warning(
                        "Expected a 'break' statement before 'default'.",
                        token);
                }
                indentation(-option.indent);
                advance('default');
                g = true;
                advance(':');
                break;
            case '}':
                indent -= option.indent;
                indentation();
                advance('}', t);
                if (this.cases.length === 1 || this.condition.id === 'true' ||
                        this.condition.id === 'false') {
                    warning("This 'switch' should be an 'if'.", this);
                }
                funct['(breakage)'] -= 1;
                funct['(verb)'] = undefined;
                return;
            case '(end)':
                error("Missing '{a}'.", nexttoken, '}');
                return;
            default:
                if (g) {
                    switch (token.id) {
                    case ',':
                        error("Each value should have its own case label.");
                        return;
                    case ':':
                        statements();
                        break;
                    default:
                        error("Missing ':' on a case clause.", token);
                    }
                } else {
                    error("Expected '{a}' and instead saw '{b}'.",
                        nexttoken, 'case', nexttoken.value);
                }
            }
        }
    }).labelled = true;

    stmt('debugger', function () {
        if (!option.debug) {
            warning("All 'debugger' statements should be removed.");
        }
    });

    stmt('do', function () {
        funct['(breakage)'] += 1;
        funct['(loopage)'] += 1;
        block(true);
        advance('while');
        var t = nexttoken;
        nonadjacent(token, t);
        advance('(');
        nospace();
        parse(20);
        if (nexttoken.id === '=') {
            warning("Expected a conditional expression and instead saw an assignment.");
            advance('=');
            parse(20);
        }
        advance(')', t);
        nospace(prevtoken, token);
        funct['(breakage)'] -= 1;
        funct['(loopage)'] -= 1;
    }).labelled = true;

    blockstmt('for', function () {
        var s, t = nexttoken;
        funct['(breakage)'] += 1;
        funct['(loopage)'] += 1;
        advance('(');
        nonadjacent(this, t);
        nospace();
        if (peek((nexttoken.id === 'var' || nexttoken.id === 'const' ) ? 1 : 0).id === 'in') {
            if (nexttoken.id === 'var') {
                advance('var');
                varstatement(true);
            } 
			else if (nexttoken.id === 'const') {
                advance('const');
                varstatement(true);
            }
			else {
                advance();
            }
            advance('in');
            parse(20);
            advance(')', t);
            s = block(true);
            if (!option.forin && (s.length > 1 || typeof s[0] !== 'object' ||
                    s[0].value !== 'if')) {
                warning("The body of a for in should be wrapped in an if statement to filter unwanted properties from the prototype.", this);
            }
            funct['(breakage)'] -= 1;
            funct['(loopage)'] -= 1;
            return this;
        } else {
            if (nexttoken.id !== ';') {
                if (nexttoken.id === 'var') {
                    advance('var');
                    varstatement();
                }
				else if (nexttoken.id === 'const') {
                    advance('const');
                    varstatement();
                }
				else {
                    for (;;) {
                        parse(0, 'for');
                        if (nexttoken.id !== ',') {
                            break;
                        }
                        advance(',');
                    }
                }
            }
            advance(';');
            if (nexttoken.id !== ';') {
                parse(20);
                if (nexttoken.id === '=') {
                    warning("Expected a conditional expression and instead saw an assignment.");
                    advance('=');
                    parse(20);
                }
            }
            advance(';');
            if (nexttoken.id === ';') {
                error("Expected '{a}' and instead saw '{b}'.",
                        nexttoken, ')', ';');
            }
            if (nexttoken.id !== ')') {
                for (;;) {
                    parse(0, 'for');
                    if (nexttoken.id !== ',') {
                        break;
                    }
                    advance(',');
                }
            }
            advance(')', t);
            nospace(prevtoken, token);
            block(true);
            funct['(breakage)'] -= 1;
            funct['(loopage)'] -= 1;
        }
    }).labelled = true;


    stmt('break', function () {
        var v = nexttoken.value;
        if (funct['(breakage)'] === 0) {
            warning("Unexpected '{a}'.", nexttoken, this.value);
        }
        nolinebreak(this);
        if (nexttoken.id !== ';') {
            if (token.line === nexttoken.line) {
                if (funct[v] !== 'label') {
                    warning("'{a}' is not a statement label.", nexttoken, v);
                } else if (scope[v] !== funct) {
                    warning("'{a}' is out of scope.", nexttoken, v);
                }
                advance();
            }
        }
        reachable('break');
    });


    stmt('continue', function () {
        var v = nexttoken.value;
        if (funct['(breakage)'] === 0) {
            warning("Unexpected '{a}'.", nexttoken, this.value);
        }
        nolinebreak(this);
        if (nexttoken.id !== ';') {
            if (token.line === nexttoken.line) {
                if (funct[v] !== 'label') {
                    warning("'{a}' is not a statement label.", nexttoken, v);
                } else if (scope[v] !== funct) {
                    warning("'{a}' is out of scope.", nexttoken, v);
                }
                advance();
            }
        }
        reachable('continue');
    });


    stmt('return', function () {
        nolinebreak(this);
        if (nexttoken.id === '(regexp)') {
            warning("Wrap the /regexp/ literal in parens to disambiguate the slash operator.");
        }
        if (nexttoken.id !== ';' && !nexttoken.reach) {
            nonadjacent(token, nexttoken);
            parse(20);
        }
        reachable('return');
    });


    stmt('throw', function () {
        nolinebreak(this);
        nonadjacent(token, nexttoken);
        parse(20);
        reachable('throw');
    });

    reserve('void');

//  Superfluous reserved words

    reserve('class');
//    reserve('const');
    reserve('enum');
    reserve('export');
    reserve('extends');
    reserve('float');
    reserve('goto');
    reserve('import');
    reserve('let');
    reserve('super');

    function jsonValue() {

        function jsonObject() {
            var t = nexttoken;
            advance('{');
            if (nexttoken.id !== '}') {
                for (;;) {
                    if (nexttoken.id === '(end)') {
                        error("Missing '}' to match '{' from line {a}.",
                                nexttoken, t.line + 1);
                    } else if (nexttoken.id === '}') {
                        warning("Unexpected comma.", token);
                        break;
                    } else if (nexttoken.id === ',') {
                        error("Unexpected comma.", nexttoken);
                    } else if (nexttoken.id !== '(string)') {
                        warning("Expected a string and instead saw {a}.",
                                nexttoken, nexttoken.value);
                    }
                    advance();
                    advance(':');
                    jsonValue();
                    if (nexttoken.id !== ',') {
                        break;
                    }
                    advance(',');
                }
            }
            advance('}');
        }

        function jsonArray() {
            var t = nexttoken;
            advance('[');
            if (nexttoken.id !== ']') {
                for (;;) {
                    if (nexttoken.id === '(end)') {
                        error("Missing ']' to match '[' from line {a}.",
                                nexttoken, t.line + 1);
                    } else if (nexttoken.id === ']') {
                        warning("Unexpected comma.", token);
                        break;
                    } else if (nexttoken.id === ',') {
                        error("Unexpected comma.", nexttoken);
                    }
                    jsonValue();
                    if (nexttoken.id !== ',') {
                        break;
                    }
                    advance(',');
                }
            }
            advance(']');
        }

        switch (nexttoken.id) {
        case '{':
            jsonObject();
            break;
        case '[':
            jsonArray();
            break;
        case 'true':
        case 'false':
        case 'null':
        case '(number)':
        case '(string)':
            advance();
            break;
        case '-':
            advance('-');
            if (token.character !== nexttoken.from) {
                warning("Unexpected space after '-'.", token);
            }
            adjacent(token, nexttoken);
            advance('(number)');
            break;
        default:
            error("Expected a JSON value.", nexttoken);
        }
    }


// The actual JSLINT function itself.

    var itself = function (s, o) {
        var a, i;
        JSLINT.errors = [];
        predefined = Object.create(standard);
        if (o) {
            a = o.predef;
            if (a instanceof Array) {
                for (i = 0; i < a.length; i += 1) {
                    predefined[a[i]] = true;
                }
            }
            if (o.adsafe) {
                o.safe = true;
            }
            if (o.safe) {
                o.browser = false;
                o.css     = false;
                o.debug   = false;
                o.eqeqeq  = true;
                o.evil    = false;
                o.forin   = false;
                o.nomen   = true;
                o.on      = false;
                o.rhino   = false;
                o.safe    = true;
                o.sidebar = false;
                o.strict  = true;
                o.sub     = false;
                o.undef   = true;
                o.widget  = false;
                predefined.Date = false;
                predefined['eval'] = false;
                predefined.Function = false;
                predefined.Object = false;
                predefined.ADSAFE = true;
            }
            option = o;
        } else {
            option = {};
        }
        option.indent = option.indent || 4;
        adsafe_id = '';
        adsafe_may = false;
        adsafe_went = false;
        approved = {};
        if (option.approved) {
            for (i = 0; i < option.approved.length; i += 1) {
                approved[option.approved[i]] = option.approved[i];
            }
        }
        approved.test = 'test'; ///////////////////////////////////////
        tab = '';
        for (i = 0; i < option.indent; i += 1) {
            tab += ' ';
        }
        indent = 0;
        global = Object.create(predefined);
        scope = global;
        funct = {
            '(global)': true,
            '(name)': '(global)',
            '(scope)': scope,
            '(breakage)': 0,
            '(loopage)': 0
        };
        functions = [];
        ids = {};
        urls = [];
        src = false;
        xmode = false;
        stack = null;
        member = {};
        membersOnly = null;
        implied = {};
        inblock = false;
        lookahead = [];
        jsonmode = false;
        warnings = 0;
        lex.init(s);
        prereg = true;

        prevtoken = token = nexttoken = syntax['(begin)'];
        assume();

        try {
            advance();
            if (nexttoken.value.charAt(0) === '<') {
                html();
                if (option.adsafe && !adsafe_went) {
                    warning("ADsafe violation: Missing ADSAFE.go.", this);
                }
            } else {
                switch (nexttoken.id) {
                case '{':
                case '[':
                    option.laxbreak = true;
                    jsonmode = true;
                    jsonValue();
                    break;
                case '@':
                case '*':
                case '#':
                case '.':
                case ':':
                    xmode = 'style';
                    advance();
                    if (token.id !== '@' || !nexttoken.identifier ||
                            nexttoken.value !== 'charset') {
                        error('A css file should begin with @charset "UTF-8";');
                    }
                    advance();
                    if (nexttoken.type !== '(string)' &&
                            nexttoken.value !== 'UTF-8') {
                        error('A css file should begin with @charset "UTF-8";');
                    }
                    advance();
                    advance(';');
                    styles();
                    break;

                default:
                    if (option.adsafe && option.fragment) {
                        warning("ADsafe violation.", this);
                    }
                    statements('lib');
                }
            }
            advance('(end)');
        } catch (e) {
            if (e) {
                JSLINT.errors.push({
                    reason    : e.message,
                    line      : e.line || nexttoken.line,
                    character : e.character || nexttoken.from
                }, null);
            }
        }
        return JSLINT.errors.length === 0;
    };

    function to_array(o) {
        var a = [], k;
        for (k in o) {
            if (o.hasOwnProperty(k)) {
                a.push(k);
            }
        }
        return a;
    }

// Report generator.

    itself.reportObj = function (option, sep) {
        var a = [], c, e, f, i, k, l, m = '', n, o = [], s, v, cl, va, un, ou, gl, la;
		var rpt={};

        rpt.Functions={};

        if (urls.length > 0) {
			rpt.URLs=urls;
        }

        s = to_array(scope).sort();
        if (s.length === 0) {
            if (jsonmode) {
                if (k === 0) {
					rpt.JSON='good';
                } else {
					rpt.JSON='bad';
                }
            }
        } else {
			rpt.Globals=[];
			for(i=0; i<s.length; i++) {
				rpt.Globals.push(s[i]);
			}
        }

		if(functions.length > 0) {
			rpt.Functions=[];
		}
        for (i = 0; i < functions.length; i += 1) {
            f = functions[i];
			func = {};
            cl = [];
            va = [];
            un = [];
            ou = [];
            gl = [];
            la = [];

			func.name=f['(name)'];
			func.line=f['(line)'];
			func.params=f['(params)'];
            for (k in f) {
                if (f.hasOwnProperty(k) && k.charAt(0) !== '(') {
                    v = f[k];
                    switch (v) {
                    case 'closure':
                        cl.push(k);
                        break;
                    case 'var':
                        va.push(k);
                        break;
                    case 'const':
                        va.push(k);
                        break;
                    case 'unused':
                        un.push(k);
                        break;
                    case 'label':
                        la.push(k);
                        break;
                    case 'outer':
                        ou.push(k);
                        break;
                    case true:
                        gl.push(k);
                        break;
                    }
                }
            }
			if(cl.length) {
				func.Closure=cl;
			}
			if(va.length) {
				func.Variables=va;
			}
			if(ou.length) {
				func.Outer=ou;
			}
			if(gl.length) {
				func['Uses Globals']=gl;
			}
			if(un.length) {
				func.Unused=un;
			}
			if(la.length) {
				func.Labels=la;
			}
			rpt.Functions.push(func);
        }
        a = [];
        for (k in member) {
            if (typeof member[k] === 'number') {
                a.push(k);
            }
        }
        if (a.length) {
            a = a.sort();
            rpt.Members = '/*members ';
            l = 10;
            for (i = 0; i < a.length; i += 1) {
                k = a[i];
                n = k.name();
                if (l + n.length > 72) {
					rpt.Members += '\r\n';
                    rpt.Members += '    ';
                    l = 1;
                }
                l += n.length + 2;
                if (member[k] === 1) {
                    n = '*' + n + '*';	// TODO: Why is this italics?
                }
                if (i < a.length - 1) {
                    n += ', ';
                }
                rpt.Members += n;
            }
			rpt.Members += '\r\n*/';
        }
        return rpt;
    };
    itself.report = function (option, sep) {
        var a = [], c, e, f, i, k, l, m = '', n, o = [], s, v, cl, va, un, ou, gl, la;

        function detail(h, s, sep) {
            if (s.length) {
                o.push('<div><i>' + h + '</i> ' +
                        s.sort().join(sep || ', ') + '</div>');
            }
        }

        s = to_array(implied);

        k = JSLINT.errors.length;
        if (k || s.length > 0) {
            o.push('<div id=errors><i>Error:</i>');
            if (s.length > 0) {
                s.sort();
                for (i = 0; i < s.length; i += 1) {
                    s[i] = '<code>' + s[i] + '</code>&nbsp;<i>' +
                        implied[s[i]].join(' ') +
                        '</i>';
                }
                o.push('<p><i>Implied global:</i> ' + s.join(', ') + '</p>');
                c = true;
            }
            for (i = 0; i < k; i += 1) {
                c = JSLINT.errors[i];
                if (c) {
                    e = c.evidence || '';
                    o.push('<p>Problem' + (isFinite(c.line) ? ' at line ' + (c.line + 1) +
                            ' character ' + (c.character + 1) : '') +
                            ': ' + c.reason.entityify() +
                            '</p><p class=evidence>' +
                            (e && (e.length > 80 ? e.slice(0, 77) + '...' :
                            e).entityify()) + '</p>');
                }
            }
            o.push('</div>');
            if (!c) {
                return o.join('');
            }
        }

        if (!option) {

            o.push('<br><div id=functions>');

            if (urls.length > 0) {
                detail("URLs<br>", urls, '<br>');
            }

            s = to_array(scope);
            if (s.length === 0) {
                if (jsonmode) {
                    if (k === 0) {
                        o.push('<p>JSON: good.</p>');
                    } else {
                        o.push('<p>JSON: bad.</p>');
                    }
                } else {
                    o.push('<div><i>No new global variables introduced.</i></div>');
                }
            } else {
                o.push('<div><i>Global</i> ' + s.sort().join(', ') + '</div>');
            }

            for (i = 0; i < functions.length; i += 1) {
                f = functions[i];
                cl = [];
                va = [];
                un = [];
                ou = [];
                gl = [];
                la = [];
                for (k in f) {
                    if (f.hasOwnProperty(k) && k.charAt(0) !== '(') {
                        v = f[k];
                        switch (v) {
                        case 'closure':
                            cl.push(k);
                            break;
                        case 'var':
                            va.push(k);
                            break;
                        case 'const':
                            va.push(k);
                            break;
                        case 'unused':
                            un.push(k);
                            break;
                        case 'label':
                            la.push(k);
                            break;
                        case 'outer':
                            ou.push(k);
                            break;
                        case true:
                            gl.push(k);
                            break;
                        }
                    }
                }
                o.push('<br><div class=function><i>' + f['(line)'] + '</i> ' +
                        (f['(name)'] || '') + '(' +
                        (f['(params)'] || '') + ')</div>');
                detail('Closure', cl);
                detail('Variable', va);
                detail('Outer', ou);
                detail('Global', gl);
                detail('<big><b>Unused</b></big>', un);
                detail('Label', la);
            }
            a = [];
            for (k in member) {
                if (typeof member[k] === 'number') {
                    a.push(k);
                }
            }
            if (a.length) {
                a = a.sort();
                m = '<br><pre>/*members ';
                l = 10;
                for (i = 0; i < a.length; i += 1) {
                    k = a[i];
                    n = k.name();
                    if (l + n.length > 72) {
                        o.push(m + '<br>');
                        m = '    ';
                        l = 1;
                    }
                    l += n.length + 2;
                    if (member[k] === 1) {
                        n = '<i>' + n + '</i>';
                    }
                    if (i < a.length - 1) {
                        n += ', ';
                    }
                    m += n;
                }
                o.push(m + '<br>*/</pre>');
            }
            o.push('</div>');
        }
        return o.join('');
    };

    return itself;

}();

function SYNCJSLINT(argc, argv)
{
	var already_loaded={};
	function LOADFILE(lines, index, pos, fname, paths, options)
	{
		var i;
		var tmp;
		var offset=0;

		if(options.multiload) {
			if(already_loaded[fname] !== undefined) {
				writeln("Skipping "+fname);
				return(0);
			}
		}
		already_loaded[fname]=true;
		if(!file_exists(fname)) {
			tmp=file_getname(fname);
			for(i=0; i<paths.length; i++) {
				if(file_exists(paths[i]+tmp)) {
					fname=paths[i]+tmp;
					break;
				}
			}
		}

		var all_lines;
		var f=new File(fname);
		var m;

		writeln("Loading "+fname);
		if(f.open("r",65535)) {
			all_lines=f.readAll();
			for(i=0; i<all_lines.length; i++) {
				lines.splice(pos+offset+i, 0, all_lines[i]);
				index.splice(pos+offset+i, 0, fname+":"+(i+1));
	
				/* TODO: smart string parsing... */
				tmp=all_lines[i];
				tmp=tmp.replace(/\/\*.*?\*\//g,'');
				tmp=tmp.replace(/\/\/.*^/,'');
				if((m=tmp.match(/^\s*load\([^"']*['"](.*)['"]\)/))!==null) {
					offset+=LOADFILE(lines,index,pos+offset+i,m[1],paths,options);
				}
	
			}
			f.close();
			return(all_lines.length+offset);
		}
		else {
			writeln("!!Load failed");
		}
		return(0);
	}

	var paths=[backslash(system.exec_dir)];
	var all_lines;
	var index;
	var options={cap:true,evil:true,laxbreak:true,newcap:true,nomen:true,undef:true,multiload:true,noradix:true,noescapement:true,poorrelations:true,noliteral:true,noextracomma:true};
	var myResult;
	var tmpVar1;
	var tmpVar2;
	var tmpVar3;
	var optdone=false;

	function dumparray(obj,indent) {
		var prop;
		var ind=format("%*s",indent,'');

		writeln(ind+obj.sort().join("\r\n"+ind));
	}

	function dumpobjs(obj,indent) {
		var prop;
		var ind=format("%*s",indent,'');

		if(obj.constructor.toString().substr(0,16) == 'function Array()')
			dumparray(obj,indent);
		else {
			for(prop in obj) {
				if(typeof(obj[prop])=='object') {
					writeln(ind+prop+":");
					if(obj[prop].constructor.toString().substr(0,16) == 'function Array()')
						dumparray(obj[prop],indent+2);
					else
						dumpobjs(obj[prop],indent+2);
				}
				else {
					writeln(ind+prop+": "+obj[prop]);
				}
			}
		}
	}

	function dumpFunc(obj) {
		var prop;
		var indent=2;
		var ind=format("%*s",indent,'');

		writeln(ind+obj.name+"("+(obj.params===undefined?'':obj.params)+"):");
		indent+=2;
		ind=format("%*s",indent,'');
		writeln(ind+index[obj.line]);
		for(prop in obj) {
			switch(prop) {
				case 'name':
				case 'line':
				case 'params':
					break;
				default:
					if(typeof(obj[prop])=='object') {
						writeln(ind+prop+":");
						dumpobjs(obj[prop],indent+2);
					}
					else {
						writeln(ind+prop+": "+obj[prop]);
					}
			}
		}
	}

	function dumpFuncs(obj) {
		var f;
		var indent=0;
		var ind=format("%*s",indent,'');

		if(obj !== undefined) {
			writeln("Functions:");
			for(f in obj) {
				dumpFunc(obj[f]);
				writeln('');
			}
		}
	}

	function dumpGlobals(obj) {
		var gl;

		if(obj !== undefined) {
			writeln('Defined Globals:');
			writeln('  '+obj.sort().join("\r\n  "));
		}
	}

	function dumpURLs(obj) {
		var gl;

		if(obj !== undefined) {
			writeln('URLs:');
			writeln('  '+obj.sort().join("\r\n  "));
		}
	}

	function dumpJSON(obj) {
		if(obj !== undefined) {
			writeln('JSON: '+obj);
		}
	}

	for(tmpVar1 in argv) {
		if(tmpVar1.search(/^[0-9]+$/)!=-1) {
			if(!optdone) {
				if(argv[tmpVar1]=='--') {
					optdone=true;
					continue;
				}
				if(argv[tmpVar1].substr(0,3)=='-L:') {
					myResult=argv[tmpVar1].substr(3).split(/:/);
					for(tmpVar2 in myResult) {
						if(tmpVar2.search(/^[0-9]+$/)!=-1) {
							paths.push(backslash(myResult[tmpVar2]));
						}
					}
					continue;
				}
				if(argv[tmpVar1].charAt(0)=='-') {
					if(options[argv[tmpVar1].substr(1)] === undefined)
						options[argv[tmpVar1].substr(1)]=true;
					else
						options[argv[tmpVar1].substr(1)]=!options[argv[tmpVar1].substr(1)];
					continue;
				}
			}

			all_lines=[];
			index=[];
			LOADFILE(all_lines, index, 0, argv[tmpVar1], paths, options);
			writeln("Linting...");
			myResult=JSLINT(all_lines,options);
			if(!myResult) {
				for(tmpVar2 in JSLINT.errors) {
					if(tmpVar2.search(/^[0-9]+$/)!=-1) {
						if(JSLINT.errors[tmpVar2]==undefined)
							continue;
						writeln("-----");
						writeln(index[JSLINT.errors[tmpVar2].line]+"@"+JSLINT.errors[tmpVar2].character+": "+JSLINT.errors[tmpVar2].reason);
						writeln(JSLINT.errors[tmpVar2].evidence);
						writeln(format("%.*s",JSLINT.errors[tmpVar2].character,JSLINT.errors[tmpVar2].evidence).replace(/[^\x00-\x1f]/g,' ')+'^');
					}
				}
			}

			if(options.report) {
				myResult=JSLINT.reportObj();

				if(myResult===undefined) {
					writeln("Report NOT generated");
				}
				else {
					dumpGlobals(myResult.Globals);
					writeln('');
					dumpFuncs(myResult.Functions);
					dumpURLs(myResult.URLs);
					dumpJSON(myResult.JSON);
					//dumpobjs(myResult,0);
				}
			}
		}
	}
}

SYNCJSLINT(argc, argv);
