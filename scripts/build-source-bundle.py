#!/usr/bin/env python3
"""Build deterministic read-only source module plus documented ARM64 overlays."""
import hashlib
import io
import json
from pathlib import Path
import re
import sys
import tarfile
from arm64_source import translate_docfile, translate_debug_compat, extract_source_dates, extract_string_utilities, translate_budget_strings, translate_span_worker_cleanup, translate_keepaway_nearest_ball, translate_xcaliber_shot_return, translate_xcaliber_draw_visibility, translate_xcaliber_key_descriptors, translate_xcaliber_fire_trace, translate_xcaliber_escape_scan, translate_xcaliber_held_input, translate_tothefront_draw, translate_tothefront_init, translate_tothefront_player_pick, translate_tothefront_ai, translate_tothefront_sentinel, extract_keyboard_helpers, translate_math, translate_conversion_math, translate_f32_math, translate_vec3_math, translate_vec4_math, translate_mat4_math, translate_ndarray_math, translate_math_test_dependencies, translate_sysrep_processor, translate_cpurep_arm64, translate_pick3d_class_terminator, translate_classmeta_callback, translate_mandel_single_cpu, translate_lighttable_memory_prompt, translate_make_tos_dependency, translate_make_home_dependency, translate_legacy_kernel_header, translate_tos_config_boot_memory, translate_memrep_arm64, translate_profiler_idle_sentinel, translate_interrupt_demo_svc, translate_andnotmod_benchmark, translate_pci_interrupts_gic_note, translate_multicore_interrupts_cooperative, translate_multicore_radix_single_cpu, translate_blackdiamond_math_dependency, translate_elephant_walk_default_dc, translate_compile_demo, translate_cartesian_plot_color, translate_low_pass_filter_sampling, translate_polezeros_math_dependency, translate_symmetry_message_masks, translate_home_localize, translate_home_wrappers
from arm64_source import (extract_god_timer_pick, translate_palette_editor,
                          translate_fonted_draw_callback,
                          translate_wallpaper_compiler_stat, translate_wallpaper_desktop)

from arm64_source import (extract_home_set, extract_home_startup, extract_home_startup_tasks, inject_home_startup_tasks, extract_version_scalars, extract_source_prompt, extract_source_training, extract_server_task, extract_user_task, extract_ktask_stack_grow, translate_doctree, translate_registry, translate_docterm, translate_doced, translate_docfind, translate_docform, translate_docpopup, translate_docrecalc, translate_grbitmap,
                          translate_autocomplete_task, translate_grdc, translate_grscreen, translate_ktask, translate_mathode, translate_zsplash,
                          translate_winmgr, translate_win_focus_trace, translate_job_focus_trace, translate_sched_focus_trace, translate_filemgr, translate_home_once,
                          translate_grpalette, translate_message,
                          translate_stock_game_math_calls,
                          translate_empty_label_statement,
                          translate_wenceslas, translate_progress,
                          translate_ring3_svc,
                          inject_sprite_elem_base_sizes,
                          inject_sprite_interpolation, inject_sprite_matrix_draw,
                          inject_sprite_wrappers)

root = Path(__file__).resolve().parent.parent
manifest = json.loads((root / 'upstream/pinned/UPSTREAM.json').read_text())
entries = {}
home_startup_tasks = None
for name, expected in manifest['files'].items():
    rel = name.removeprefix('src/')
    data = (root / 'upstream/pinned' / rel).read_bytes()
    if hashlib.sha256(data).hexdigest() != expected:
        raise SystemExit(f'Pinned source changed: {name}')
    if rel in ('Kernel/KernelB.HH', 'Kernel/KernelC.HH'):
        try:
            data = translate_legacy_kernel_header(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 legacy kernel header overlay no longer applies: {exc}') from exc
    if rel == 'System/Sound.ZC':
        # The music bridge supplies shared globals; reuse the pinned noise and
        # sweep tasks after the kernel audio and task interfaces are bound.
        marker = b'#define SE_NOISE\t\t0\n'
        if data.count(marker) != 1:
            raise SystemExit('pinned sound effects section changed')
        entries['System/SoundEffects.ZC'] = data[data.index(marker):]
    if rel == 'Demo/Graphics/FontEd.ZC':
        data = translate_fonted_draw_callback(data)
    if rel == 'System/God/HolySpirit.ZC':
        entries['System/God/TimerPick.ZC'] = extract_god_timer_pick(data)
    if rel == 'Home/PaletteEditor/PaletteGod.ZC':
        data = b'#include "/System/God/TimerPick.ZC"\n' + data
    if rel == 'Home/PaletteEditor/PaletteEdit.ZC':
        data = translate_palette_editor(data)
    if rel == 'System/WallPaper.ZC':
        try:
            data = translate_wallpaper_compiler_stat(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 wallpaper compiler-stat overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Graphics/WallPapers/WallPaperDesktop.ZC':
        try:
            data = translate_wallpaper_desktop(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 desktop wallpaper overlay no longer applies: {exc}') from exc
    if rel == 'Demo/AcctExample/HomeKeyPlugIns.ZC':
        # The stock sample assumes the interactive picker always returns a
        # path. The ARM64 bootstrap currently supports cancellation without a
        # selected path, so keep these optional hotkeys safe on that target.
        replacements = (
            (b'\tU8 *st = PopUpPickFile;\n\n\tst[0] =',
             b'\tU8 *st = PopUpPickFile;\n\n\tif (!st)\n\t\treturn;\n\n\tst[0] ='),
            (b'\tU8\t\t\t*st = PopUpPickDir, *st2;\n\n\tst[0] =',
             b'\tU8\t\t\t*st = PopUpPickDir, *st2;\n\n\tif (!st)\n\t\treturn;\n\n\tst[0] ='),
        )
        for needle, replacement in replacements:
            if data.count(needle) != 1:
                raise SystemExit('ARM64 HomeKeyPlugIns null-picker guard overlay no longer applies')
            data = data.replace(needle, replacement)
    if rel == 'Demo/Lectures/AndNotMod.ZC':
        try:
            data = translate_andnotmod_benchmark(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 AndNotMod benchmark overlay no longer applies: {exc}') from exc
    if rel == 'HomeSys.ZC':
        entries['System/HomeStartUp.ZC'] = extract_home_startup(data)
        home_startup_tasks = extract_home_startup_tasks(data)
    if rel == 'System/Debug.ZC':
        try:
            kernel_debug = (root / 'upstream/pinned/Kernel/KDebug.ZC').read_bytes()
            data = translate_debug_compat(data, kernel_debug)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Debug overlay no longer applies: {exc}') from exc
    if rel == 'Apps/Budget/BgtStrs.ZC':
        data = translate_budget_strings(data)
    if rel == 'Demo/Graphics/Cartesian.ZC':
        try:
            data = translate_cartesian_plot_color(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Cartesian display overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Graphics/LowPassFilter.ZC':
        try:
            data = translate_low_pass_filter_sampling(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 LowPassFilter responsiveness overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Graphics/PoleZeros.ZC':
        try:
            data = translate_polezeros_math_dependency(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 PoleZeros math dependency overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Graphics/Symmetry.ZC':
        try:
            data = translate_symmetry_message_masks(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Symmetry input overlay no longer applies: {exc}') from exc
    if rel == 'System/Math/Math.ZC':
        data = translate_math(data)
    if rel == 'System/Math/Conversion.ZC':
        try:
            data = translate_conversion_math(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 conversion-math overlay no longer applies: {exc}') from exc
    if rel == 'System/Math/F32.ZC':
        try:
            data = translate_f32_math(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 F32 math overlay no longer applies: {exc}') from exc
    if rel == 'System/Math/Vec4.ZC':
        try:
            data = translate_vec4_math(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Vec4 math overlay no longer applies: {exc}') from exc
    if rel == 'System/Math/Vec3.ZC':
        try:
            data = translate_vec3_math(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Vec3 math overlay no longer applies: {exc}') from exc
    if rel == 'System/Math/Mat4.ZC':
        try:
            data = translate_mat4_math(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Mat4 math overlay no longer applies: {exc}') from exc
    if rel == 'System/Math/NDArray.ZC':
        try:
            data = translate_ndarray_math(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 NDArray math overlay no longer applies: {exc}') from exc
    if rel in ('System/Math/Tests/TestVec.ZC',
               'System/Math/Tests/TestMat4.ZC',
               'System/Math/Tests/TestF32.ZC'):
        try:
            data = translate_math_test_dependencies(data, rel)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Math test dependency overlay no longer applies: {exc}') from exc
    if rel == 'Home/Net/Protocols/TCP/TCP.ZC':
        # TCPConnectionCreate returns a socket pointer, so represent connect
        # failure as NULL rather than leaking TCPSocketConnect's negative
        # status code through a pointer-typed API.
        needle = (b'\t\t\t\tTCPSocketClose(result);\n'
                  b'\t\t\t\treturn error;\n'
                  b'\t\t\t}\n\n'
                  b'\t\t\treturn result;')
        replacement = (b'\t\t\t\tTCPSocketClose(result);\n'
                       b'\t\t\t\treturn NULL;\n'
                       b'\t\t\t}\n\n'
                       b'\t\t\treturn result;')
        if data.count(needle) != 1:
            raise SystemExit('ARM64 TCP connection failure overlay expected one error return')
        data = data.replace(needle, replacement)
        # QueueRemove unlinks an entry and clears its next/last pointers in
        # the ARM64 kernel bridge. TCPAcknowledgePacket must save the successor
        # before unlinking the ACK queue entry, or the next ACK follows NULL.
        needle = (b'\t\t\tQueueRemove(send_buffer);\n'
                  b'\t\t\tFree(send_buffer->tcp_packet);\n\n'
                  b'\t\t\tsend_buffer_temp = send_buffer->next; // QueueRemove doesn\'t change removed queue\'s links')
        replacement = (b'\t\t\tsend_buffer_temp = send_buffer->next; // Save successor before QueueRemove unlinks it.\n'
                       b'\t\t\tQueueRemove(send_buffer);\n'
                       b'\t\t\tFree(send_buffer->tcp_packet);')
        if data.count(needle) != 1:
            raise SystemExit('ARM64 TCP ACK queue overlay no longer applies')
        data = data.replace(needle, replacement)
        # The stock network programs are loaded as separate ZealC units, so
        # the socket entry points they call must be exported from the stack.
        # Upstream's MakeTCP include makes these visible only while compiling
        # the stack itself; the ARM64 program loader resolves public symbols.
        for name in (b'TCPSocket', b'TCPSocketBind', b'TCPSocketClose',
                     b'TCPSocketConnect', b'TCPSocketListen',
                     b'TCPSocketAccept', b'TCPSocketReceive',
                     b'TCPSocketReceiveLine', b'TCPSocketSend',
                     b'TCPSocketSendAll', b'TCPSocketSendString',
                     b'TCPConnectionCreate'):
            pattern = re.compile(rb'(?m)^((?:I64\s+|CTCPSocket\s+\*?\s*)'
                                 + re.escape(name) + rb'\s*\()')
            data, count = pattern.subn(rb'public \1', data)
            if count != 1:
                raise SystemExit('ARM64 TCP public API overlay expected one '
                                 + name.decode() + ' definition, found '
                                 + str(count))
    if rel == 'Home/Net/Utilities/Net.HH':
        interrupt = b'DefinePrint("I_NETHANDLER", "%d", IntEntryAlloc);'
        if data.count(interrupt) != 1:
            raise SystemExit('ARM64 network interrupt allocation anchor changed')
        data = data.replace(interrupt,
                            b'// The ARM64 network queue dispatches without an interrupt vector.')
    if rel == 'Home/Net/Programs/Gopher.ZC':
        # The stock Gopher client stores CTCPSocket pointers in I64 locals.
        # The ARM64 runtime resolves external calls with typed signatures, so
        # passing those integers to the pointer-typed TCP API leaves calls
        # unresolved. Keep socket handles pointer-typed end to end.
        replacements = (
            (b'I64 GopherOpen(', b'CTCPSocket *GopherOpen('),
            (b'\t\treturn -1;\n\t}\n\t\n//\tsock = create_connection(host, port);',
             b'\t\treturn NULL;\n\t}\n\t\n//\tsock = create_connection(host, port);'),
            (b'\tI64 sock;\n', b'\tCTCPSocket *sock;\n'),
            (b'I64\t\t data_len = 0, total_len = 0, got, sock;',
             b'I64\t\t data_len = 0, total_len = 0, got;\n\tCTCPSocket *sock;'),
            (b'\tI64 sock, n;', b'\tCTCPSocket *sock;\n\tI64 n;'),
            (b'\tU8 *line;\n\tCTCPSocket *sock;',
             b'\tU8 line[4096];\n\tI64 selector_len, query_len = 0, pos, i;\n\tCTCPSocket *sock;'),
            (b'\tif (query == NULL)\n\t{\n\t\tline = StrPrint(NULL, "%s\\r\\n", selector);\n\t}\n\telse\n\t{\n\t\tline = StrPrint(NULL, "%s\\t%s\\r\\n", selector, query);\n\t}',
             b'\tselector_len = 0;\n'
             b'\twhile (selector[selector_len]) selector_len++;\n'
             b'\tif (query) while (query[query_len]) query_len++;\n'
             b'\tif (query && selector_len + query_len + 4 > sizeof(line)) return -1;\n'
             b'\tif (!query && selector_len + 3 > sizeof(line)) return -1;\n'
             b'\tfor (pos = 0; pos < selector_len; pos++) line[pos] = selector[pos];\n'
             b'\tif (query)\n\t{\n'
             b'\t\tline[pos++] = \'\\t\';\n'
             b'\t\tfor (i = 0; i < query_len; i++) line[pos++] = query[i];\n\t}\n'
             b'\tline[pos++] = \'\\r\';\n'
             b'\tline[pos++] = \'\\n\';\n'
             b'\tline[pos] = 0;'),
            (b'\tFree(line);', b'\t// Request buffer is local to GopherOpen.'),
        )
        for needle, replacement in replacements:
            if data.count(needle) != 1:
                raise SystemExit('ARM64 Gopher socket type overlay expected one '
                                 + repr(needle.decode()) + ' occurrence')
            data = data.replace(needle, replacement)
        if data.count(b'if (sock <= 0)') != 3:
            raise SystemExit('ARM64 Gopher pointer-null overlay expected three socket checks')
        data = data.replace(b'if (sock <= 0)', b'if (sock == NULL)')
        needle = b'U0 GopherLine(U8 *line)'
        if data.count(needle) != 1:
            raise SystemExit('ARM64 Gopher tab parser overlay expected one GopherLine')
        data = data.replace(
            needle,
            b'U8 *GopherFindTab(U8 *text)\n'
            b'{\n'
            b'\twhile (*text && *text != \'\\t\') text++;\n'
            b'\tif (*text) return text;\n'
            b'\treturn NULL;\n'
            b'}\n\n'
            + needle)
        if data.count(b'StrFind("\\t", line)') != 3:
            raise SystemExit('ARM64 Gopher tab parser overlay expected three StrFind calls')
        data = data.replace(b'StrFind("\\t", line)', b'GopherFindTab(line)')
        needle = b'U0 GopherLine(U8 *line)'
        data = data.replace(
            needle,
            b'U0 GopherPrintEscaped(U8 *txt, U8 *backslashes = "")\n'
            b'{\n'
            b'\twhile (*txt)\n'
            b'\t{\n'
            b'\t\tif (*txt == \'$\' && txt[1] == \'$\')\n'
            b'\t\t{\n'
            b'\t\t\t"$$$$";\n'
            b'\t\t\ttxt += 2;\n'
            b'\t\t}\n'
            b'\t\telse\n'
            b'\t\t{\n'
            b'\t\t\tif ((*txt == \'\\\\\' || *txt == \'"\') && *backslashes)\n'
            b'\t\t\t\t"%s", backslashes;\n'
            b'\t\t\t"%c", *txt++;\n'
            b'\t\t}\n'
            b'\t}\n'
            b'}\n\n'
            + needle)
        for token in (b'PrintEscaped(display);',
                      b'PrintEscaped(display, "\\\\");',
                      b'PrintEscaped(host, "\\\\\\\\\\\\");',
                      b'PrintEscaped(selector, "\\\\\\\\\\\\");'):
            if data.count(token) != 1:
                raise SystemExit('ARM64 Gopher escaped-output overlay expected one '
                                 + repr(token.decode()) + ' call')
            data = data.replace(token, token.replace(b'PrintEscaped(',
                                                    b'GopherPrintEscaped('))
        needle = b'PrintErr("Failed to connect to %s:%d\\n", host, port);'
        if data.count(needle) != 1:
            raise SystemExit('ARM64 Gopher connect diagnostic overlay expected one PrintErr')
        data = data.replace(needle,
                            b'Print("Gopher connect failed for %s:%d\\n", host, port);')
    if rel == 'Home/Net/Programs/TCPChatServer.ZC':
        if data.count(b'U0 ChatServer()') != 1:
            raise SystemExit('ARM64 TCP chat server export overlay expected one ChatServer')
        data = data.replace(b'U0 ChatServer()', b'public U0 ChatServer(I64 port=0)')
        needle = (b'\tU8\t\t\t\t\t*port_string = StrGet("Server Port: ");\n'
                  b'\tI64\t\t\t\t\t port = Str2I64(port_string);')
        replacement = (b'\tU8\t\t\t\t\t*port_string;\n'
                       b'\tif (!port)\n'
                       b'\t{\n'
                       b'\t\tport_string = StrGet("Server Port: ");\n'
                       b'\t\tport = Str2I64(port_string);\n'
                       b'\t\tFree(port_string);\n'
                       b'\t}')
        if data.count(needle) != 1:
            raise SystemExit('ARM64 TCP chat server optional port overlay no longer applies')
        data = data.replace(needle, replacement)
        if data.count(b'\n\n\tFree(port_string);\n') != 1:
            raise SystemExit('ARM64 TCP chat server port cleanup overlay expected one cleanup')
        data = data.replace(b'\n\n\tFree(port_string);\n', b'\n')
        replacement = (b'\tCTCPSocket\t\t\t*conn_socket;\n\n'
                       b'\tif (tcp == NULL) tcp = TCPSocket(AF_INET);\n'
                       b'\tif (connections == NULL) connections = CAlloc(sizeof(CTCPTreeQueue));\n'
                       b'\tif (connections->next == NULL) QueueInit(connections);\n'
                       b'\tif (buffer == NULL) buffer = CAlloc(ETHERNET_FRAME_SIZE);')
        # Global initializers are skipped when the app source is loaded with
        # CCF_JUST_LOAD. Ensure the exported entry point can initialize itself
        # when invoked later from another task.
        # The preceding cleanup was already moved into the interactive branch.
        anchor = b'\tCTCPSocket\t\t\t*conn_socket;\n\n\n'
        if data.count(anchor) != 1:
            raise SystemExit('ARM64 TCP chat server initialization anchor no longer applies')
        data = data.replace(anchor, replacement)
        if data.count(b'if (new_socket > 0)') != 1:
            raise SystemExit('ARM64 TCP chat server pointer check overlay expected one check')
        data = data.replace(b'if (new_socket > 0)', b'if (new_socket != NULL)')
        helper_anchor = b'\nU0 ChatServerKill()'
        if data.count(helper_anchor) != 1:
            raise SystemExit('ARM64 TCP chat IPv4 helper anchor no longer applies')
        helper = (b'\nU32 ChatServerPeerIPv4(CTCPSocket *socket)\n'
                  b'{\n'
                  b'\tCSocketAddressIPV4 *address = &socket->destination_address;\n'
                  b'\treturn address->address.address;\n'
                  b'}\n'
                  b'\nU0 ChatServerKill()')
        data = data.replace(helper_anchor, helper)
        for variable in (b'dest_socket', b'tcp_socket', b'socket'):
            address = variable + b'->destination_address(CSocketAddressIPV4).address.address'
            if data.count(address) != 1:
                raise SystemExit('ARM64 TCP chat IPv4 access overlay expected one '
                                 +variable.decode()+' access')
            data = data.replace(address, b'ChatServerPeerIPv4('+variable+b')')
    if rel == 'Home/Net/Programs/TCPChatClient.ZC':
        parse_anchor = b'\nU0 Chat()\n'
        if data.count(parse_anchor) != 1:
            raise SystemExit('ARM64 TCP chat client IPv4 parser overlay expected one Chat entry point')
        parser = (b'\nBool ChatClientParseIPv4(U8 *text, U32 *destination)\n'
                  b'{\n'
                  b'\tU8 *cursor = text;\n'
                  b'\tU32 value = 0;\n'
                  b'\tI64 i, octet, digits;\n\n'
                  b'\tfor (i = 0; i < 4; i++)\n'
                  b'\t{\n'
                  b'\t\toctet = 0;\n'
                  b'\t\tdigits = 0;\n'
                  b'\t\twhile (*cursor >= \'0\' && *cursor <= \'9\')\n'
                  b'\t\t{\n'
                  b'\t\t\toctet = octet * 10 + (*cursor - \'0\');\n'
                  b'\t\t\tif (octet > 255) return FALSE;\n'
                  b'\t\t\tdigits++;\n'
                  b'\t\t\tcursor++;\n'
                  b'\t\t}\n'
                  b'\t\tif (!digits) return FALSE;\n'
                  b'\t\tvalue |= octet << (i * 8);\n'
                  b'\t\tif (i < 3)\n'
                  b'\t\t{\n'
                  b'\t\t\tif (*cursor != \'.\') return FALSE;\n'
                  b'\t\t\tcursor++;\n'
                  b'\t\t}\n'
                  b'\t}\n'
                  b'\tif (*cursor) return FALSE;\n'
                  b'\t*destination = EndianU32(value);\n'
                  b'\treturn TRUE;\n'
                  b'}\n'
                  b'\nU0 Chat()\n')
        data = data.replace(parse_anchor, parser)
        anchor = (b'\tI64\t\t\t\t\t receive_val;\n\n'
                  b'\twhile (PresentationToNetwork(AF_INET, ip_string, &ip_addr) == -1)')
        replacement = (b'\tI64\t\t\t\t\t receive_val;\n\n'
                       b'\t// CCF_JUST_LOAD skips the stock top-level socket initializer.\n'
                       b'\tif (tcp == NULL) tcp = TCPSocket(AF_INET);\n\n'
                       b'\twhile (!ChatClientParseIPv4(ip_string, &ip_addr.address))')
        if data.count(anchor) != 1:
            raise SystemExit('ARM64 TCP chat client lazy socket initialization overlay no longer applies')
        data = data.replace(anchor, replacement)
        if data.count(b'\tU8\t\t\t\t\t*buffer[ETHERNET_FRAME_SIZE];') != 1:
            raise SystemExit('ARM64 TCP chat client receive buffer overlay expected one declaration')
        data = data.replace(b'\tU8\t\t\t\t\t*buffer[ETHERNET_FRAME_SIZE];',
                            b'\tU8\t\t\t\t\t buffer[ETHERNET_FRAME_SIZE + 1];')
        receive_anchor = b'\t\t\telse if (receive_val > 0)\n\t\t\t{\n'
        if data.count(receive_anchor) != 1:
            raise SystemExit('ARM64 TCP chat client receive terminator overlay expected one branch')
        data = data.replace(receive_anchor,
                            receive_anchor + b'\t\t\t\tbuffer[receive_val] = 0;\n')
    if rel == 'Home/Net/Programs/UDPChat.ZC':
        globals_anchor = (b'U8\t\t\t\t*dest_ip_string\t\t= StrGet("Destination IPV4: "),\n'
                          b'\t\t\t\t*port_string\t\t= StrGet("RX & TX UDP Port: ");\n'
                          b'CIPV4Address\t*dest_address\t\t= CAlloc(sizeof(CIPV4Address));\n\n'
                          b'I64\t\t\t\t port\t\t\t\t= Str2I64(port_string);')
        globals_replacement = (b'U8\t\t\t\t*dest_ip_string\t\t= NULL,\n'
                               b'\t\t\t\t*port_string\t\t= NULL;\n'
                               b'CIPV4Address\t*dest_address\t\t= NULL;\n\n'
                               b'I64\t\t\t\t port\t\t\t\t= 0;')
        if data.count(globals_anchor) != 1:
            raise SystemExit('ARM64 UDP chat lazy prompt initialization overlay no longer applies')
        data = data.replace(globals_anchor, globals_replacement)
        chat_start = b'\nU0 Chat()\n'
        chat_end = b'\nChat;'
        if data.count(chat_start) != 1 or data.count(chat_end) != 1:
            raise SystemExit('ARM64 UDP chat entry point overlay expected one Chat definition and launch')
        start = data.index(chat_start)
        end = data.index(chat_end, start)
        chat = (b'\nBool UDPChatParseIPv4(U8 *text, U32 *destination)\n'
                b'{\n'
                b'\tU8 *cursor = text;\n'
                b'\tU32 value = 0;\n'
                b'\tI64 i, octet, digits;\n\n'
                b'\tfor (i = 0; i < 4; i++)\n'
                b'\t{\n'
                b'\t\toctet = 0;\n'
                b'\t\tdigits = 0;\n'
                b'\t\twhile (*cursor >= \'0\' && *cursor <= \'9\')\n'
                b'\t\t{\n'
                b'\t\t\toctet = octet * 10 + (*cursor - \'0\');\n'
                b'\t\t\tif (octet > 255) return FALSE;\n'
                b'\t\t\tdigits++;\n'
                b'\t\t\tcursor++;\n'
                b'\t\t}\n'
                b'\t\tif (!digits) return FALSE;\n'
                b'\t\tvalue |= octet << (i * 8);\n'
                b'\t\tif (i < 3)\n'
                b'\t\t{\n'
                b'\t\t\tif (*cursor != \'.\') return FALSE;\n'
                b'\t\t\tcursor++;\n'
                b'\t\t}\n'
                b'\t}\n'
                b'\tif (*cursor) return FALSE;\n'
                b'\t*destination = EndianU32(value);\n'
                b'\treturn TRUE;\n'
                b'}\n'
                b'\nU0 Chat(U8 *probe_ip = NULL, I64 probe_port = 0)\n'
                b'{\n'
                b'\tCUDPSocket\t\t\t*udp_socket;\n'
                b'\tCSocketAddressIPV4\t*sock_addr;\n'
                b'\tU8\t\t\t\t\t buffer[ETHERNET_FRAME_SIZE + 1];\n'
                b'\tI64\t\t\t\t\t receive_count;\n\n'
                b'\tif (probe_ip != NULL) dest_ip_string = probe_ip;\n'
                b'\tif (probe_port) port = probe_port;\n'
                b'\tif (dest_ip_string == NULL) dest_ip_string = StrGet("Destination IPV4: ");\n'
                b'\tif (dest_address == NULL) dest_address = CAlloc(sizeof(CIPV4Address));\n'
                b'\twhile (dest_ip_string && !UDPChatParseIPv4(dest_ip_string, &dest_address->address))\n'
                b'\t{\n'
                b'\t\t"ERROR: Bad IP entered. Retry.\\n";\n'
                b'\t\tdest_ip_string = StrGet("Destination IPV4: ");\n'
                b'\t}\n'
                b'\tif (port == 0)\n'
                b'\t{\n'
                b'\t\tport_string = StrGet("RX & TX UDP Port: ");\n'
                b'\t\tport = Str2I64(port_string);\n'
                b'\t}\n'
                b'\tif (port <= 0 || port > 65535)\n'
                b'\t{\n'
                b'\t\t"ERROR: UDP port must be from 1 through 65535.\\n";\n'
                b'\t\treturn;\n'
                b'\t}\n'
                b'\tudp_socket = UDPSocket(AF_INET);\n'
                b'\tsock_addr = CAlloc(sizeof(CSocketAddressIPV4));\n'
                b'\tMemSet(buffer, 0, sizeof(buffer));\n'
                b'\tChatInit;\n\n'
                b'\t// Bind wildcard local address, not the remote peer address.\n'
                b'\tsock_addr->port = EndianU16(port);\n'
                b'\tsock_addr->family = AF_INET;\n'
                b'\tsock_addr->address.address = 0;\n'
                b'\tif (UDPSocketBind(udp_socket, sock_addr) < 0)\n'
                b'\t{\n'
                b'\t\tDocPrint(chat_display_task->put_doc, "Unable to bind UDP port %d\\n", port);\n'
                b'\t\tKill(chat_message_task);\n'
                b'\t\tKill(chat_display_task);\n'
                b'\t\tUDPSocketClose(udp_socket);\n'
                b'\t\tFree(sock_addr);\n'
                b'\t\treturn;\n'
                b'\t}\n'
                b'\twhile (TaskValidate(chat_message_task))\n'
                b'\t{\n'
                b'\t\treceive_count = UDPSocketReceiveFrom(udp_socket, buffer, '
                b'sizeof(buffer) - 1, NULL);\n'
                b'\t\tif (receive_count > 0)\n'
                b'\t\t{\n'
                b'\t\t\tbuffer[receive_count] = 0;\n'
                b'\t\t\tDocBottom(chat_display_task->put_doc);\n'
                b'\t\t\tDocPrint(chat_display_task->put_doc, '
                b'"$$BG,PURPLE$$$$BLACK$$<%s>$$FG$$$$BG$$  %s\\n", '
                b'dest_ip_string, buffer);\n'
                b'\t\t\tMemSet(buffer, 0, sizeof(buffer));\n'
                b'\t\t}\n'
                b'\t\tRefresh;\n'
                b'\t}\n'
                b'\tUDPSocketClose(udp_socket);\n'
                b'\tFree(sock_addr);\n'
                b'}')
        data = data[:start] + chat + data[end:]
        # CCF_JUST_LOAD intentionally leaves the stock top-level Chat launch inert.
        if data.count(b'\nChat;') != 1:
            raise SystemExit('ARM64 UDP chat launch suppression anchor expected one Chat call')
    if rel == 'Home/Net/Programs/FTPClient.ZC':
        message_anchor = b'I64 FTPMessageGet(CTCPSocket *message_socket, U8 *buf)\n'
        passive_anchor = b'I64 FTPReplyPassiveParse(CTCPSocket *message_socket, CSocketAddressIPV4 *dest_addr_out)\n'
        if data.count(message_anchor) != 1 or data.count(passive_anchor) != 1:
            raise SystemExit('ARM64 FTP message reader overlay expected one function')
        message_start = data.index(message_anchor)
        message_end = data.index(passive_anchor, message_start)
        message = data[message_start:message_end]
        reader = (b'I64 FTPMessageGet(CTCPSocket *message_socket, U8 *buf)\n'
                  b'{ // returns FTP status code of message\n'
                  b'\tI64 status = 0, received, i;\n'
                  b'\tBool first = TRUE;\n\n'
                  b'\tMemSet(buf, 0, FTP_BUF_SIZE);\n'
                  b'\t"\\n";\n'
                  b'\twhile ((received = TCPSocketReceive(message_socket, buf, '
                  b'FTP_BUF_SIZE - 1)) > 0)\n'
                  b'\t{\n'
                  b'\t\tbuf[received] = 0;\n'
                  b'\t\tif (first)\n'
                  b'\t\t{\n'
                  b'\t\t\tif (buf[0] >= \'0\' && buf[0] <= \'9\' && '
                  b'buf[1] >= \'0\' && buf[1] <= \'9\' && '
                  b'buf[2] >= \'0\' && buf[2] <= \'9\')\n'
                  b'\t\t\t\tstatus = (buf[0] - \'0\') * 100 + '
                  b'(buf[1] - \'0\') * 10 + (buf[2] - \'0\');\n'
                  b'\t\t\tfirst = FALSE;\n'
                  b'\t\t}\n'
                  b'\t\tfor (i = 0; i < received; i++)\n'
                  b'\t\t\tif (buf[i] == \'\\r\') buf[i] = \'\\n\';\n'
                  b'\t\t"%s", buf;\n'
                  b'\t\tMemSet(buf, 0, FTP_BUF_SIZE);\n'
                  b'\t}\n'
                  b'\t"\\n";\n'
                  b'\treturn status;\n'
                  b'}\n\n')
        if message.count(message_anchor) != 1:
            raise SystemExit('ARM64 FTP bounded reply-buffer overlay expected one reader')
        message = reader
        data = data[:message_start] + message + data[message_end:]
        parse_anchor = b'I64 FTPReplyPassiveParse(CTCPSocket *message_socket, CSocketAddressIPV4 *dest_addr_out)\n'
        download_anchor = b'\nI64 FTPFileDownload(CTCPSocket *data_socket, U8 *dest)\n'
        if data.count(parse_anchor) != 1 or data.count(download_anchor) != 1:
            raise SystemExit('ARM64 FTP PASV parser overlay expected one function boundary')
        start = data.index(parse_anchor)
        end = data.index(download_anchor, start)
        passive_parser = (b'I64 FTPReplyPassiveParse(CTCPSocket *message_socket, '
                          b'CSocketAddressIPV4 *dest_addr_out)\n'
                          b'{\n'
                          b'\tU8 buffer[8192], *cursor;\n'
                          b'\tI64 received, received_sum = 0, field, fields[6], i;\n'
                          b'\tU32 ip_addr = 0;\n'
                          b'\tU16 port;\n\n'
                          b'\tMemSet(buffer, 0, sizeof(buffer));\n'
                          b'\twhile ((received = TCPSocketReceive(message_socket, '
                          b'buffer + received_sum, sizeof(buffer) - received_sum - 1)) > 0)\n'
                          b'\t{\n'
                          b'\t\treceived_sum += received;\n'
                          b'\t\tif (received_sum >= sizeof(buffer) - 1) break;\n'
                          b'\t}\n'
                          b'\tbuffer[received_sum] = 0;\n'
                          b'\tcursor = buffer;\n'
                          b'\twhile (*cursor && *cursor != \'(\') cursor++;\n'
                          b'\tif (*cursor != \'(\') return -1;\n'
                          b'\tcursor++;\n'
                          b'\tfor (i = 0; i < 6; i++)\n'
                          b'\t{\n'
                          b'\t\tfield = 0;\n'
                          b'\t\tif (*cursor < \'0\' || *cursor > \'9\') return -1;\n'
                          b'\t\twhile (*cursor >= \'0\' && *cursor <= \'9\')\n'
                          b'\t\t{\n'
                          b'\t\t\tfield = field * 10 + (*cursor - \'0\');\n'
                          b'\t\t\tif (field > 255) return -1;\n'
                          b'\t\t\tcursor++;\n'
                          b'\t\t}\n'
                          b'\t\tfields[i] = field;\n'
                          b'\t\tif (i < 5)\n'
                          b'\t\t{\n'
                          b'\t\t\tif (*cursor != \',\') return -1;\n'
                          b'\t\t\tcursor++;\n'
                          b'\t\t}\n'
                          b'\t}\n'
                          b'\tfor (i = 0; i < 4; i++) ip_addr |= fields[i] << (i * 8);\n'
                          b'\tport = fields[4] * 256 + fields[5];\n'
                          b'\tdest_addr_out->family = AF_INET;\n'
                          b'\tdest_addr_out->port = EndianU16(port);\n'
                          b'\tdest_addr_out->address.address = EndianU32(ip_addr);\n'
                          b'\treturn 0;\n'
                          b'}\n')
        data = data[:start] + passive_parser + data[end:]
        ftp_anchor = b'I64 FTPClient(U8 *hostname=NULL, U16 port=21)\n'
        if data.count(ftp_anchor) != 1:
            raise SystemExit('ARM64 FTP numeric IPv4 parser insertion point expected once')
        helper = (b'Bool FTPClientParseIPv4(U8 *text, U32 *destination)\n'
                  b'{\n'
                  b'\tU8 *cursor = text;\n'
                  b'\tU32 value = 0;\n'
                  b'\tI64 i, octet, digits;\n\n'
                  b'\tfor (i = 0; i < 4; i++)\n'
                  b'\t{\n'
                  b'\t\toctet = 0;\n'
                  b'\t\tdigits = 0;\n'
                  b'\t\twhile (*cursor >= \'0\' && *cursor <= \'9\')\n'
                  b'\t\t{\n'
                  b'\t\t\toctet = octet * 10 + (*cursor - \'0\');\n'
                  b'\t\t\tif (octet > 255) return FALSE;\n'
                  b'\t\t\tdigits++;\n'
                  b'\t\t\tcursor++;\n'
                  b'\t\t}\n'
                  b'\t\tif (!digits) return FALSE;\n'
                  b'\t\tvalue |= octet << (i * 8);\n'
                  b'\t\tif (i < 3)\n'
                  b'\t\t{\n'
                  b'\t\t\tif (*cursor != \'.\') return FALSE;\n'
                  b'\t\t\tcursor++;\n'
                  b'\t\t}\n'
                  b'\t}\n'
                  b'\tif (*cursor) return FALSE;\n'
                  b'\t*destination = EndianU32(value);\n'
                  b'\treturn TRUE;\n'
                  b'}\n\n'
                  b'I64 FTPClient(U8 *hostname=NULL, U16 port=21)\n')
        data = data.replace(ftp_anchor, helper)
        ipv4_anchor = b'if (!IPV4AddressParse(hostname, &addr))'
        if data.count(ipv4_anchor) != 1:
            raise SystemExit('ARM64 FTP numeric IPv4 parser call site expected once')
        data = data.replace(ipv4_anchor,
                            b'if (!FTPClientParseIPv4(hostname, &addr))')
        socket_decl = (b'CTCPSocket\t\t\t*message_socket = TCPSocket(AF_INET), *data_socket;')
        if data.count(socket_decl) != 1:
            raise SystemExit('ARM64 FTP socket declaration overlay expected once')
        data = data.replace(socket_decl,
                            b'CTCPSocket\t\t\t*message_socket, *data_socket;')
        timeout_anchor = b'\tmessage_socket->timeout = TCP_TIMEOUT;'
        if data.count(timeout_anchor) != 1:
            raise SystemExit('ARM64 FTP socket initialization point expected once')
        data = data.replace(timeout_anchor,
                            b'\tmessage_socket = TCPSocket(AF_INET);\n'
                            b'\tif (!message_socket) return -1;\n'
                            + timeout_anchor)
        connected_text = b'"\\nSuccessfully connected.\\n";'
        if data.count(connected_text) != 1:
            raise SystemExit('ARM64 FTP connection status string expected once')
        data = data.replace(connected_text,
                            b'"\\nFTP control connection established.\\n";')
    if rel == 'Home/Net/Programs/Telnet/Extra/TelnetGr.ZC':
        for old, new in (
                (b'#include "TelnetNegotiation"',
                 b'#include "../TelnetNegotiation"'),
                (b'#include "TelnetHelpers"',
                 b'#include "../TelnetHelpers"')):
            if data.count(old) != 1:
                raise SystemExit('ARM64 TelnetGr sibling-include overlay expected once: '
                                 +repr(old))
            data = data.replace(old, new)
        class_anchor = b'  I64 bgcolor;\n}\nclass Terminal {'
        if data.count(class_anchor) != 1:
            raise SystemExit('ARM64 TelnetGr ScreenCell class terminator expected once')
        data = data.replace(class_anchor,
                            b'  I64 bgcolor;\n};\nclass Terminal {')
    if rel == 'Home/Net/Tests/TCPTest0.ZC':
        if data.count(b'\nNetStop;\n') != 1:
            raise SystemExit('ARM64 TCPTest0 NetStop call overlay expected once')
        # NetStop belongs to the legacy port-I/O drivers. The ARM64 network
        # path is VirtIO-net, whose loader owns device lifetime and exposes no
        # matching global shutdown operation.
        data = data.replace(b'\nNetStop;\n',
                            b'\n// NetStop is unavailable on the ARM64 VirtIO-net path.\n')
    if rel == 'Home/Net/Programs/Telnet/TelnetHelpers.ZC':
        start = b'I64 ANSIArtLoad(U8 *filename, U8 *buffer) {'
        end = b'\n}\n\npublic U8 *ANSIArtBrowser()'
        if data.count(start) != 1 or data.count(end) != 1:
            raise SystemExit('ARM64 Telnet ANSI art loader overlay expected once')
        body = (b'I64 ANSIArtLoad(U8 *filename, U8 *buffer)\n'
                b'{\n'
                b'  I64 size;\n'
                b'  U8 *file_buffer = FileRead(filename, &size);\n'
                b'  if (!file_buffer) return -1;\n'
                b'  if (size < 0 || size >= BUF_SIZE)\n'
                b'  {\n'
                b'    Free(file_buffer);\n'
                b'    return -1;\n'
                b'  }\n'
                b'  MemCopy(buffer, file_buffer, size);\n'
                b'  Free(file_buffer);\n'
                b'  return size;\n'
                b'}')
        begin = data.index(start)
        finish = data.index(end, begin) + 2
        data = data[:begin] + body + data[finish:]
        prompt = (b'U0 TelnetPrompt(CHostForm *form) {\n'
                  b'  form->host[0] = 0;\n  form->port = TELNET_PORT;\n'
                  b'  PopUpForm(form);\n}')
        if data.count(prompt) != 1:
            raise SystemExit('ARM64 Telnet connection form overlay expected once')
        data = data.replace(prompt, prompt.replace(b'U0 TelnetPrompt', b'Bool TelnetPrompt')
                            .replace(b'PopUpForm(form);',
                                     b'return PopUpForm(form, "CHostForm") && *form->host && form->port;'))
    if rel == 'Home/Net/Programs/Telnet/TelnetClass.ZC':
        if data.count(b'  I64 sock;') != 1:
            raise SystemExit('ARM64 Telnet socket field overlay expected once')
        data = data.replace(b'  I64 sock;', b'  CTCPSocket *sock;')
    if rel == 'Home/Net/Programs/Telnet/TelnetNegotiation.ZC':
        for signature in (b'U0 SendWindowSize(I64 sock, U16 cols, U16 rows)',
                          b'U0 SendTerminalType(I64 sock)',
                          b'U0 TelnetNegotiate(I64 sock, U8 *ptr)'):
            if data.count(signature) != 1:
                raise SystemExit('ARM64 Telnet socket parameter overlay expected once: '
                                 +repr(signature))
            data = data.replace(signature, signature.replace(b'I64 sock',
                                                              b'CTCPSocket *sock'))
    if rel == 'Home/Net/Programs/Telnet/Telnet.ZC':
        replacements = (
            (b'U0 Telnet(U8 *host=NULL, U16 port=TELNET_PORT) {\n  CHostForm form;',
             b'U0 Telnet(U8 *host=NULL, U16 port=TELNET_PORT) {\n'
             b'  force_disconnect = FALSE;\n  CHostForm form;'),
            (b'I64 TelnetOpen(U8 *host, U16 port)',
             b'CTCPSocket *TelnetOpen(U8 *host, U16 port)'),
            (b'    term.sock_ready = 1; // Signal that the socket is ready\n'
             b'    term.waiting_for_input = FALSE;\n\n'
             b'    "$$BG,GREEN$$$$WHITE$$Connected$$FG$$$$BG$$\\n";\n'
             b'    Sleep(1000);\n'
             b'    DocClear;\n',
             b'    "$$BG,GREEN$$$$WHITE$$Connected$$FG$$$$BG$$\\n";\n'
             b'    Sleep(1000);\n'
             b'    DocClear;\n'
             b'    term.waiting_for_input = FALSE;\n'
             b'    term.sock_ready = TRUE;\n'
             b'    term.task = Spawn(&TerminalTask, NULL, "Telnet Receiver", -1, Fs);\n'
             b'    if (!term.task)\n'
             b'    {\n'
             b'      TelnetDisconnect;\n'
             b'      PopUpOk("Could not start Telnet receiver.\\n");\n'
             b'      term.waiting_for_input = TRUE;\n'
             b'      host = NULL;\n'
             b'      goto show_splash;\n'
             b'    }\n'),
            (b'    // if(term.sock != NULL) TCPSocketClose(term.sock);\n'
             b'    // TODO: should probably kill the task if it already exists too?\n\n'
             b'    // Spawn a task to receive data from the socket\n'
             b'    term.task = Spawn(&TerminalTask, NULL, "Telnet");\n\n',
             b'    TelnetDisconnect;\n'),
            (b'            TelnetPrompt(&form);\n',
             b'            if (!TelnetPrompt(&form)) break;\n'),
            (b'              term.sock_ready = 0;\n              term.waiting_for_input = TRUE;',
             b'              TelnetDisconnect;\n              term.waiting_for_input = TRUE;'),
            (b'              term.sock_ready = 0;\n              host = NULL;\n'
             b'              TCPSocketClose(term.sock);',
             b'              host = NULL;\n              TelnetDisconnect;'),
            (b'  // ideally go back to the splashscreen and wait for another input?\n'
             b'  // term.sock_ready = 0;\n'
             b'  // term.waiting_for_input = TRUE;\n'
             b'  // goto show_splash;', b'  TelnetDisconnect;'),
            (b'    return -1;\n  }\n  DocPrint(term.doc,',
             b'    return NULL;\n  }\n  DocPrint(term.doc,'),
            (b'  I64 sock;\n\n  if (host == NULL)',
             b'  CTCPSocket *sock;\n'
             b'  CSocketAddressIPV4 address;\n'
             b'  U8 *cursor;\n'
             b'  U32 ip = 0;\n'
                        b'  I64 i, octet, digits, connect_result;\n'
             b'  Bool numeric = TRUE;\n\n'
             b'  if (host == NULL)'),
            (b'if (term.sock <= 0)', b'if (term.sock == NULL)'),
            (b'if (sock <= 0)', b'if (sock == NULL)'),
        )
        for old, new in replacements:
            if data.count(old) != 1:
                raise SystemExit('ARM64 Telnet socket typing overlay expected one anchor: '
                                 +repr(old))
            data = data.replace(old, new)
        receiver_start = b'U0 TerminalTask() {'
        receiver_end = b'\n\nU0 Telnet(U8 *host=NULL'
        if data.count(receiver_start) != 1 or data.count(receiver_end) != 1:
            raise SystemExit('ARM64 Telnet receiver overlay expected once')
        begin = data.index(receiver_start)
        finish = data.index(receiver_end, begin)
        data = data[:begin] + b'''U0 TerminalTask()
{
  while (term.sock_ready)
  {
    term.buffer_len = TCPSocketReceive(term.sock, term.buffer, BUF_SIZE - 1);
    if (term.buffer_len == 0)
    {
      term.sock_ready = FALSE;
      DocPrint(term.doc, "Connection closed by the remote host.\\n");
      break;
    }
    if (term.buffer_len > 0)
    {
      term.buffer[term.buffer_len] = 0;
      ANSIParse;
    }
  }
  term.task = NULL;
}

U0 TelnetDisconnect()
{
  term.sock_ready = FALSE;
  if (term.task)
  {
    Kill(term.task, TRUE);
    term.task = NULL;
  }
  if (term.sock)
  {
    TCPSocketClose(term.sock);
    term.sock = NULL;
  }
}''' + data[finish:]
        connect_anchor = b'  sock = TCPConnectionCreate(host, port);'
        connect_body = (b'  cursor = host;\n'
                        b'  for (i = 0; i < 4; i++)\n'
                        b'  {\n'
                        b'    octet = 0;\n'
                        b'    digits = 0;\n'
                        b'    while (*cursor >= \'0\' && *cursor <= \'9\')\n'
                        b'    {\n'
                        b'      octet = octet * 10 + (*cursor - \'0\');\n'
                        b'      if (octet > 255) numeric = FALSE;\n'
                        b'      digits++;\n'
                        b'      cursor++;\n'
                        b'    }\n'
                        b'    if (!digits) numeric = FALSE;\n'
                        b'    ip |= octet << (i * 8);\n'
                        b'    if (i < 3)\n'
                        b'    {\n'
                        b'      if (*cursor != \'.\') numeric = FALSE;\n'
                        b'      else cursor++;\n'
                        b'    }\n'
                        b'  }\n'
                        b'  if (*cursor) numeric = FALSE;\n'
                        b'  if (numeric)\n'
                        b'  {\n'
                        b'    address.family = AF_INET;\n'
                        b'    address.port = EndianU16(port);\n'
                        b'    address.address.address = EndianU32(ip);\n'
                        b'    sock = TCPSocket(AF_INET);\n'
                        b'    if (sock) sock->timeout = TCP_TIMEOUT * 3;\n'
                        b'    if (sock) connect_result = TCPSocketConnect(sock, &address);\n'
                        b'    else connect_result = -1;\n'
                        b'    if (sock && connect_result != 0)\n'
                        b'    {\n'
                        b'      TCPSocketClose(sock);\n'
                        b'      sock = NULL;\n'
                        b'    }\n'
                        b'  }\n'
                        b'  else\n'
                        b'    sock = TCPConnectionCreate(host, port);')
        if data.count(connect_anchor) != 1:
            raise SystemExit('ARM64 Telnet numeric IPv4 connection overlay expected once')
        data = data.replace(connect_anchor, connect_body)
    if rel == 'Home/Net/Protocols/TCP/TCPHandler.ZC':
        needle = b'QueueSize(tcp_socket->accept_queue)'
        if data.count(needle) != 1:
            raise SystemExit('ARM64 TCP listen backlog overlay expected one accept-queue check')
        # The listen backlog is a node count, and the ARM64 bootstrap exports
        # QueueCount but not QueueSize. QueueSize also measures allocated bytes,
        # so it was the wrong operation for this limit on every target.
        data = data.replace(needle, b'QueueCount(tcp_socket->accept_queue)')
    if rel == 'System/Utils/SysRep.ZC':
        try:
            data = translate_sysrep_processor(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 SysRep overlay no longer applies: {exc}') from exc
    if rel == 'System/Utils/CPURep.ZC':
        try:
            data = translate_cpurep_arm64(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 CPURep overlay no longer applies: {exc}') from exc
    if rel == 'System/Utils/MemRep.ZC':
        try:
            data = translate_memrep_arm64(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 MemRep overlay no longer applies: {exc}') from exc
    if rel == 'System/Utils/Profiler.ZC':
        try:
            data = translate_profiler_idle_sentinel(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Profiler overlay no longer applies: {exc}') from exc
    if rel in ('Compiler/Lex.ZC', 'Compiler/CompilerB.HH'):
        # The pinned signature stores an optional output length but the
        # implementation dereferences it. Preserve that API correctly on
        # the AArch64 source volume, including calls that omit the argument.
        if rel == 'Compiler/Lex.ZC':
            needle = b'U8 *LexFirstRemove(CCompCtrl *cc, U8 *marker, I64 _len=NULL)'
            replacement = b'U8 *LexFirstRemove(CCompCtrl *cc, U8 *marker, I64 *_len=NULL)'
        else:
            needle = b'public extern U8\t\t*LexFirstRemove(CCompCtrl *cc, U8 *marker, I64 _len=NULL);'
            replacement = b'public extern U8\t\t*LexFirstRemove(CCompCtrl *cc, U8 *marker, I64 *_len=NULL);'
        if data.count(needle) != 1:
            raise SystemExit('ARM64 LexFirstRemove pointer overlay no longer applies')
        data = data.replace(needle, replacement)
    if rel == 'Compiler/Lex.ZC':
        numeric_range = b'\t\t\tcase 128...255:'
        if data.count(numeric_range) != 1:
            raise SystemExit('ARM64 lexer byte-range overlay no longer applies')
        byte_cases = b'\n'.join(b'\t\t\tcase 0x%02X:' % value for value in range(0x80, 0x100))
        data = data.replace(numeric_range, byte_cases)
    if rel == 'Apps/Span/SpanMain.ZC':
        data = translate_span_worker_cleanup(data)
    if rel == 'Apps/KeepAway/KeepAway.ZC':
        try:
            data = translate_keepaway_nearest_ball(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 KeepAway overlay no longer applies: {exc}') from exc
    if rel == 'Demo/MultiCore/MPPrint.ZC':
        # CPU 0 is the only active worker in the current ARM64 runtime. The
        # stock color expression maps it to BLACK, invisible on the graphical
        # terminal's black canvas. Preserve per-worker colors while avoiding
        # black for every worker.
        needle = b'Gs->num & 15'
        replacement = b'(Gs->num % 15) + 1'
        if data.count(needle) != 1:
            raise SystemExit('ARM64 MPPrint visible-worker-color overlay no longer applies')
        data = data.replace(needle, replacement)
    if rel == 'Demo/Games/RainDrops.ZC':
        # The ARM64 graphical terminal uses a black canvas, while this stock
        # demo treats WHITE as its empty-space color for both the drop physics
        # and text. Give the original scene its expected white drawing surface.
        needle = (b'\tI64 w = task->pix_width, h = task->pix_height, cx = w >> 1, cy = h >> 1;'
                  b'\n\n\tdc->color = LTRED;')
        replacement = (b'\tI64 w = task->pix_width, h = task->pix_height, cx = w >> 1, cy = h >> 1;'
                       b'\n\n\tdc->color = WHITE;\n\tGrRect(dc, 0, 0, w, h);'
                       b'\n\tdc->color = LTRED;')
        if data.count(needle) != 1:
            raise SystemExit('ARM64 RainDrops canvas overlay no longer applies')
        data = data.replace(needle, replacement)
    if rel == 'Apps/X-Caliber/X-Caliber.ZC':
        try:
            data = translate_xcaliber_shot_return(data)
            data = translate_xcaliber_draw_visibility(data)
            data = translate_xcaliber_key_descriptors(data)
            data = translate_xcaliber_fire_trace(data)
            data = translate_xcaliber_escape_scan(data)
            data = translate_xcaliber_held_input(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 X-Caliber overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/BomberGolf.ZC':
        try:
            data = translate_stock_game_math_calls(
                data, blink_count=3, full_tri_count=1)
            data = translate_empty_label_statement(data, b'bm_done')
        except ValueError as exc:
            raise SystemExit(f'ARM64 BomberGolf math overlay no longer applies: {exc}') from exc
        # The x86 kernel exports Blink directly. ARM64 routes this demo through
        # Arm64Blink, which is provided by the bootstrap Math module.
        if b'#include "/System/Math/Math"' in data:
            raise SystemExit('ARM64 BomberGolf math dependency already present')
        data = b'#include "/System/Math/Math";\n' + data
    if rel == 'Demo/Games/CastleFrankenstein.ZC':
        try:
            data = translate_stock_game_math_calls(data, tri_count=1)
            data = translate_empty_label_statement(data, b'fs_done')
        except ValueError as exc:
            raise SystemExit(f'ARM64 CastleFrankenstein math overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/DunGen.ZC':
        try:
            data = translate_stock_game_math_calls(data, blink_count=1, tri_count=2)
        except ValueError as exc:
            raise SystemExit(f'ARM64 DunGen math overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/Talons.ZC':
        try:
            data = translate_stock_game_math_calls(data, tri_count=1,
                                                   sign_count=2)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Talons math overlay no longer applies: {exc}') from exc
        for name in (b'MAP_WIDTH', b'MAP_HEIGHT'):
            pattern = rb'(#define\s+' + name + rb'\s+)1024\b'
            data, count = re.subn(pattern, rb'\g<1>64', data)
            if count != 1:
                raise SystemExit(f'ARM64 Talons map-size overlay expected one '
                                 f'{name.decode()} definition, got {count}')
        panel_workers = (b'\tmp.init_not_done_flags = 1 << mp_count - 1;\n'
                         b'\tfor (i = 0; i < mp_count; i++)\n'
                         b'\t\tSpawn(&MPDoPanels, Fs, "Do Panels", i);\n'
                         b'\twhile (mp.init_not_done_flags)\n'
                         b'\t\tSleep(1);')
        single_cpu_panels = (b'\tif (mp_count == 1)\n'
                             b'\t\tMPDoPanels(Fs);\n'
                             b'\telse\n'
                             b'\t{\n'
                             b'\t\tmp.init_not_done_flags = 1 << mp_count - 1;\n'
                             b'\t\tfor (i = 0; i < mp_count; i++)\n'
                             b'\t\t\tSpawn(&MPDoPanels, Fs, "Do Panels", i);\n'
                             b'\t\twhile (mp.init_not_done_flags)\n'
                             b'\t\t\tSleep(1);\n'
                             b'\t}')
        count = data.count(panel_workers)
        if count != 1:
            raise SystemExit(f'ARM64 Talons single-CPU panel overlay expected one '
                             f'worker block, got {count}')
        data = data.replace(panel_workers, single_cpu_panels, 1)
        for old, new in (
            (b'x = MAP_WIDTH  >> 1 * COORDINATE_SCALE * MAP_SCALE;',
             b'x = (MAP_WIDTH >> 1) * COORDINATE_SCALE * MAP_SCALE;'),
            (b'y = MAP_HEIGHT >> 1 * COORDINATE_SCALE * MAP_SCALE;',
             b'y = (MAP_HEIGHT >> 1) * COORDINATE_SCALE * MAP_SCALE;'),
            (b'x -= speed * ms * COORDINATE_SCALE * dx / 1 << 16;',
             b'x -= speed * ms * COORDINATE_SCALE * dx / (1 << 16);'),
            (b'y -= speed * ms * COORDINATE_SCALE * dy / 1 << 16;',
             b'y -= speed * ms * COORDINATE_SCALE * dy / (1 << 16);'),
            (b'z -= speed * ms * COORDINATE_SCALE * dz / 1 << 16;',
             b'z -= speed * ms * COORDINATE_SCALE * dz / (1 << 16);'),
            (b'speed\t\t= 2.5;', b'speed\t\t= 0.15625;'),
        ):
            count = data.count(old)
            if count != 1:
                raise SystemExit(f'ARM64 Talons center-coordinate overlay expected one '
                                 f'{old!r}, got {count}')
            data = data.replace(old, new, 1)
        if b'#include "/System/Math/Math"' in data:
            raise SystemExit('ARM64 Talons math dependency already present')
        data = b'#include "/System/Math/Math";\n' + data
    if rel == 'Demo/Games/TheDead.ZC':
        bullets = b'\ti = bullets_out;\n\twhile (i != bullets_in)\n'
        if data.count(bullets) != 1:
            raise SystemExit('ARM64 TheDead bullet-render overlay expected one draw loop')
        data = data.replace(bullets,
                            b'\tdc->color = WHITE;\n' + bullets, 1)
    if rel == 'Demo/Games/Varoom.ZC':
        try:
            data = translate_stock_game_math_calls(data, log2_count=1)
            data = translate_empty_label_statement(data, b'vr_done')
        except ValueError as exc:
            raise SystemExit(f'ARM64 Varoom math overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/Wenceslas.ZC':
        try:
            data = translate_wenceslas(data)
            data = translate_empty_label_statement(data, b'ws_done')
        except ValueError as exc:
            raise SystemExit(f'ARM64 Wenceslas prompt overlay no longer applies: {exc}') from exc
    if rel == 'Demo/PhoneNumWords.ZC':
        # This lookup is indexed only for A-Z and does not need a string
        # terminator. Use an explicit initializer because the ARM64 compiler
        # rejects this stock string-to-array initializer form.
        needle = b'U8 char_map[26]="22233344455566677778889999";'
        replacement = (b"U8 char_map[26]={'2','2','2','3','3','3','4','4','4',"
                       b"'5','5','5','6','6','6','7','7','7','7','8','8','8',"
                       b"'9','9','9','9'};")
        if data.count(needle) != 1:
            raise SystemExit('ARM64 PhoneNumWords string extent overlay no longer applies')
        data = data.replace(needle, replacement)
    if rel == 'Demo/CompileDemo.ZC':
        try:
            data = translate_compile_demo(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 CompileDemo overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Progress.ZC':
        try:
            data = translate_progress(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Progress overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/RawHide.ZC':
        try:
            data = translate_stock_game_math_calls(data, clamp_u64_count=1)
        except ValueError as exc:
            raise SystemExit(f'ARM64 RawHide math overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/BlackDiamond.ZC':
        try:
            data = translate_blackdiamond_math_dependency(data)
            data = translate_empty_label_statement(data, b'bd_done')
        except ValueError as exc:
            raise SystemExit(f'ARM64 BlackDiamond label overlay no longer applies: {exc}') from exc
    if rel == 'Demo/ClassMeta.ZC':
        try:
            data = translate_classmeta_callback(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 ClassMeta overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Graphics/Pick3D.ZC':
        try:
            data = translate_pick3d_class_terminator(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Pick3D source overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Graphics/Mandel.ZC':
        try:
            data = translate_mandel_single_cpu(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Mandel source overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/ElephantWalk.ZC':
        try:
            data = translate_elephant_walk_default_dc(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 ElephantWalk overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Graphics/LightTable.ZC':
        try:
            data = translate_lighttable_memory_prompt(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 LightTable memory overlay no longer applies: {exc}') from exc
    if rel == 'Demo/AcctExample/TOS/MakeTOS.ZC':
        try:
            data = translate_make_tos_dependency(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 MakeTOS dependency overlay no longer applies: {exc}') from exc
    if rel == 'Demo/AcctExample/MakeHome.ZC':
        try:
            data = translate_make_home_dependency(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 MakeHome dependency overlay no longer applies: {exc}') from exc
    if rel == 'Demo/AcctExample/TOS/TOSConfig.ZC':
        try:
            data = translate_tos_config_boot_memory(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 TOSConfig boot-memory overlay no longer applies: {exc}') from exc
    if rel == 'Demo/AcctExample/HomeLocalize.ZC':
        try:
            data = translate_home_localize(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 HomeLocalize overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Lectures/InterruptDemo.ZC':
        try:
            data = translate_interrupt_demo_svc(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 InterruptDemo source overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Lectures/PCIInterrupts.ZC':
        try:
            data = translate_pci_interrupts_gic_note(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 PCIInterrupts source overlay no longer applies: {exc}') from exc
    if rel == 'Demo/MultiCore/Interrupts.ZC':
        try:
            data = translate_multicore_interrupts_cooperative(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 MultiCore/Interrupts source overlay no longer applies: {exc}') from exc
    if rel == 'Demo/MultiCore/MPRadix.ZC':
        try:
            data = translate_multicore_radix_single_cpu(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 MPRadix source overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Lectures/Ring3.ZC':
        try:
            data = translate_ring3_svc(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Ring3 source overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/ZoneOut.ZC':
        try:
            data = translate_empty_label_statement(data, b'zo_done')
        except ValueError as exc:
            raise SystemExit(f'ARM64 ZoneOut label overlay no longer applies: {exc}') from exc
    if rel == 'Apps/ToTheFront/TTFFire.ZC':
        data = translate_tothefront_sentinel(data)
    if rel == 'Apps/ToTheFront/TTFDraw.ZC':
        data = translate_tothefront_draw(data)
    if rel == 'Apps/ToTheFront/TTFInit.ZC':
        data = translate_tothefront_init(data)
    if rel in ('Apps/ToTheFront/AIs/Human.ZC',
               'Apps/ToTheFront/AIs/SimpleAI.ZC'):
        try:
            data = translate_tothefront_ai(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 ToTheFront AI overlay no longer applies: {exc}') from exc
    if rel == 'Apps/ToTheFront/ToTheFront.ZC':
        try:
            data = translate_tothefront_player_pick(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 ToTheFront player picker overlay no longer applies: {exc}') from exc
    if rel == 'Kernel/BlkDev/DiskDirB.ZC':
        entries['System/SourceHome.ZC'] = extract_home_set(data)
    if rel == 'Kernel/SerialDev/Message.ZC':
        try:
            data = translate_message(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 keyboard message overlay no longer applies: {exc}') from exc
    if rel == 'Once.ZC':
        # Seed the same editable home startup file installed by upstream.
        # Disk/RAM and DVD boot queue this file in the first graphical
        # terminal. The ARM64 DVD overlay avoids the unported BIOS/AHCI
        # installer while preserving the live session.
        entries['Home/Once.ZC'] = translate_home_once(data)
    if rel == 'Kernel/KGlobals.ZC':
        entries['System/VersionScalars.ZC'] = extract_version_scalars(data)
    if rel == 'Kernel/StrB.ZC':
        entries['System/SourcePrompt.ZC'] = extract_source_prompt(data)
    if rel == 'System/Training.ZC':
        entries['System/SourceTraining.ZC'] = extract_source_training(data)
    if rel == 'System/Gr/GrScreen.ZC':
        try:
            data = translate_grscreen(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 source overlay no longer applies: {exc}') from exc
    elif rel == 'System/Gr/GrDC.ZC':
        try:
            data = translate_grdc(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 source overlay no longer applies: {exc}') from exc
    elif rel == 'System/Gr/GrBitMap.ZC':
        try:
            data = translate_grbitmap(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 source overlay no longer applies: {exc}') from exc
    elif rel == 'System/Gr/GrPalette.ZC':
        try:
            data = translate_grpalette(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 palette overlay no longer applies: {exc}') from exc
    elif rel == 'System/WinMgr.ZC':
        try:
            data = translate_winmgr(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 window-manager overlay no longer applies: {exc}') from exc
    elif rel == 'System/Win.ZC':
        try:
            data = translate_win_focus_trace(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 window focus trace no longer applies: {exc}') from exc
    elif rel == 'Kernel/Job.ZC':
        try:
            data = translate_job_focus_trace(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 job focus trace no longer applies: {exc}') from exc
    elif rel == 'Kernel/Sched.ZC':
        try:
            data = translate_sched_focus_trace(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 scheduler focus trace no longer applies: {exc}') from exc
    elif rel == 'System/BlkDev/FileMgr.ZC':
        try:
            data = translate_filemgr(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 FileMgr overlay no longer applies: {exc}') from exc
    elif rel == 'System/Math/MathODE.ZC':
        try:
            data = translate_mathode(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 MathODE overlay no longer applies: {exc}') from exc
    elif rel == 'System/ZSplash.ZC':
        try:
            data = translate_zsplash(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 ZSplash overlay no longer applies: {exc}') from exc
    elif rel == 'System/DolDoc/DocPopUp.ZC':
        data = translate_docpopup(data)
    elif rel == 'System/DolDoc/DocForm.ZC':
        try:
            data = translate_docform(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 source overlay no longer applies: {exc}') from exc
    elif rel == 'System/DolDoc/DocFind.ZC':
        try:
            data = translate_docfind(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 source overlay no longer applies: {exc}') from exc
    elif rel == 'System/DolDoc/DocRecalc.ZC':
        try:
            data = translate_docrecalc(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 source overlay no longer applies: {exc}') from exc
    elif rel == 'System/DolDoc/DocTerm.ZC':
        data = translate_docterm(data)
    elif rel == 'System/AutoComplete/ACTask.ZC':
        try:
            data = translate_autocomplete_task(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 AutoComplete overlay no longer applies: {exc}') from exc
    elif rel == 'System/DolDoc/DocEd.ZC':
        data = translate_doced(data)
    elif rel == 'System/DolDoc/DocTree.ZC':
        data = translate_doctree(data)
    elif rel == 'System/Registry.ZC':
        data = translate_registry(data)
    elif rel == 'Kernel/KTask.ZC':
        entries['System/ServerTask.ZC'] = extract_server_task(data)
        entries['System/UserTask.ZC'] = extract_user_task(data)
        try:
            data = translate_ktask(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 KTask overlay no longer applies: {exc}') from exc
    entries[rel] = data
for path in sorted((root / 'upstream/bootstrap').rglob('*')):
    if path.is_file():
        rel = path.relative_to(root / 'upstream/bootstrap').as_posix()
        data = path.read_bytes()
        if rel == 'System/TaskBridge.ZC':
            needle = b'U8 *MStrPrint(U8 *format, ...)'
            if data.count(needle) != 1:
                raise SystemExit('ARM64 MStrPrint export overlay no longer applies')
            data = data.replace(needle, b'public ' + needle)
            # Bootstrap.ZC owns the monotonic tS() clock function. This bridge
            # placeholder global shadows that function in HolyC name lookup,
            # freezing benchmarks and window timing at zero.
            clock_placeholder = b'F64 tS = 0;'
            if data.count(clock_placeholder) != 1:
                raise SystemExit('ARM64 TaskBridge tS placeholder overlay no longer applies')
            data = data.replace(clock_placeholder,
                                b'// Use the source-backed tS() from Bootstrap.ZC.')
            try:
                data += b'\n// Stock Kernel/KTask stack-growth path on the ARM64 fiber bridge.\n'
                data += extract_ktask_stack_grow(entries['Kernel/KTask.ZC'])
            except (KeyError, ValueError) as exc:
                raise SystemExit(f'ARM64 CallStackGrow bridge no longer applies: {exc}') from exc
        if rel == 'System/Gr/SpriteBridge.ZC':
            try:
                data = inject_sprite_elem_base_sizes(
                    data, (root/'upstream/pinned/System/Gr/SpriteNew.ZC').read_bytes())
                data = inject_sprite_interpolation(
                    data,
                    (root/'upstream/pinned/System/Gr/SpriteNew.ZC').read_bytes(),
                    (root/'upstream/pinned/System/Gr/GrSpritePlot.ZC').read_bytes())
                data = inject_sprite_matrix_draw(data)
                data = inject_sprite_wrappers(
                    data, (root/'upstream/pinned/System/Gr/GrSpritePlot.ZC').read_bytes())
            except ValueError as exc:
                raise SystemExit(f'ARM64 sprite helper overlay no longer applies: {exc}') from exc
        if rel == 'System/DolDoc/DocFile.ZC':
            data = translate_docfile(data)
        entries[rel] = data
entries['System/StringUtilities.ZC'] = extract_string_utilities(
    entries['Kernel/StrA.ZC'], entries['Kernel/StrB.ZC'])
entries['System/DateBridge.ZC'] = extract_source_dates(
    entries['Kernel/KDate.ZC'], entries['Kernel/StrScan.ZC'], entries['Kernel/StrPrint.ZC'])
entries['System/KeyboardBridge.ZC'] = extract_keyboard_helpers(
    entries['Kernel/SerialDev/Keyboard.ZC'], entries['Kernel/StrPrint.ZC'])
# MakeHome's stock optional `~/...` includes fall back to root-level defaults
# when a user's /Home override is absent. Keep those unchanged upstream files
# available to the source resolver for that fallback.
for name in ('HomeLocalize.ZC', 'HomeWrappers.ZC'):
    data = (root / 'upstream/pinned' / name).read_bytes()
    if name == 'HomeLocalize.ZC':
        data = translate_home_localize(data)
    elif name == 'HomeWrappers.ZC':
        data = translate_home_wrappers(data)
    entries[name] = data
# MakeHome also resolves ~/TOS/... against the stock root-level TOS directory
# when no user-specific /Home/TOS override exists. The demo keeps its source
# copies under Demo/AcctExample/TOS; publish the same ARM64-adapted files at
# the default paths used by that include fallback.
for name in ('MakeTOS.ZC', 'TOSConfig.ZC', 'TOSDistro.ZC', 'TOSExtern.ZC',
             'TOSMisc.ZC', 'TOSTheme.ZC'):
    entries['TOS/' + name] = entries['Demo/AcctExample/TOS/' + name]
# Factory account data is the same file installed by Budget/Install.ZC. The
# source volume infers directories from their files; seed its default home
# before the unchanged installer checks for it.
entries['Home/Budget/Accts.DD'] = entries['Apps/Budget/Accts.DD']
if home_startup_tasks is not None:
    gui_terminal_path = 'System/GuiTerminal.ZC'
    gui_terminal = entries.get(gui_terminal_path)
    if gui_terminal is None:
        raise SystemExit('cannot place pinned HomeSys.StartUpTasks adapter')
    try:
        entries[gui_terminal_path] = inject_home_startup_tasks(
            gui_terminal, home_startup_tasks)
    except ValueError as exc:
        raise SystemExit(str(exc)) from exc
for path in sorted((root / 'tests/compat').glob('*')):
    if path.suffix in ('.ZC', '.HH', '.DD', '.txt'):
        entries['Tests/' + path.name] = path.read_bytes()
entries['LICENSE/ZealOS.txt'] = (root / 'upstream/pinned/LICENSE').read_bytes()
entries['LICENSE/Aiwnios.txt'] = (root / 'third_party/aiwnios/LICENSE').read_bytes()
entries['UPSTREAM.json'] = (root / 'upstream/pinned/UPSTREAM.json').read_bytes()
entries['PORTS/ARM64/GrScreen.md'] = (root / 'upstream/arm64/GrScreen.md').read_bytes()
output = Path(sys.argv[1])
output.parent.mkdir(parents=True, exist_ok=True)
with tarfile.open(output, 'w', format=tarfile.USTAR_FORMAT) as bundle:
    for name, data in sorted(entries.items()):
        info = tarfile.TarInfo(name)
        info.size = len(data)
        info.mode = 0o444
        info.mtime = info.uid = info.gid = 0
        bundle.addfile(info, io.BytesIO(data))
print(f'Source bundle: {len(entries)} files, ZealOS {manifest["commit"][:12]}')
