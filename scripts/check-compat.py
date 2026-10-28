#!/usr/bin/env python3
"""Exercise the native compiler inside a disposable QEMU/HVF guest."""
import argparse, json, os, pathlib, re, select, shlex, shutil, socket, struct, subprocess, tempfile, time, zlib
ROOT = pathlib.Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--pci', action='store_true')
p.add_argument('--usb-input', choices=('nec', 'qemu'),
               help='test native USB HID on NEC (UTM default) or QEMU xHCI, without VirtIO input')
p.add_argument('--usb-with-virtio', action='store_true',
               help='retain duplicate VirtIO devices to verify USB ownership suppresses duplicate keys')
p.add_argument('--usb-pointer', choices=('tablet', 'mouse'), default='tablet',
               help='native USB pointer layout to accept (default: tablet, also enumerate mouse)')
p.add_argument('--usb-hubs', type=int, choices=range(6), default=0,
               help='place native USB inputs behind this many nested USB2 hubs (0-5)')
p.add_argument('--usb-hotplug', action='store_true',
               help='exercise keyboard, pointer and hub detach/reattach without reboot')
p.add_argument('--quick', action='store_true', help='only compiler integration gate')
p.add_argument('--window-frame-only', action='store_true',
               help='stop after the window demo frame diagnostic')
p.add_argument('--probe-upstream', action='store_true', help='check current full-header/subsystem compiler results')
p.add_argument('--probe-module', metavar='ARCHIVE_PATH',
               help='load an unchanged upstream source after KernelA/B in a disposable guest')
p.add_argument('--probe-module-override', type=pathlib.Path,
               help='compile this host source after the selected module dependency sequence')
p.add_argument('--probe-gui-terminal', action='store_true',
               help='spawn a real DocTerm task under the live source Window Manager')
p.add_argument('--probe-gui-keyboard', action='store_true',
               help='inject printable virtio keys and verify the boot graphical terminal receives them')
p.add_argument('--probe-gui-workflow', action='store_true',
               help='verify visible graphical editing, save/compile/run, and reopen after reboot')
p.add_argument('--probe-gui-editor-text', action='store_true',
               help='isolate visible keyboard text entry, save, and run in the GUI document editor')
p.add_argument('--probe-stock-programs', action='store_true',
               help='run unchanged pinned NetOfDots and Lines in the graphical terminal')
p.add_argument('--probe-program-catalog', action='store_true',
               help='report compile-only results for original application entry points in isolated tasks')
p.add_argument('--probe-doc-includes', action='store_true',
               help='verify nested DolDoc includes, binary payload lifetime and compiler recovery')
p.add_argument('--probe-logic-program', action='store_true',
               help='exercise original Digital Logic gate sprites, solver and keyboard prompts')
p.add_argument('--probe-budget-program', action='store_true',
               help='check real clock/calendar and original Budget storage and forms')
p.add_argument('--probe-3d-apps', action='store_true',
               help='check original GrModels, KeepAway and Titanium 3D sprite services and runtime paths')
p.add_argument('--probe-stock-interactive', action='store_true',
               help='exercise unchanged upstream interactive programs with mouse and key input')
p.add_argument('--probe-lattice-controls', action='store_true',
               help='check live Lattice mouse placement/aim, colors, restart, and Space in the GUI')
p.add_argument('--probe-gui-windowdemo-input', action='store_true',
               help='isolate graphical terminal input/display across a windowdemo stop')
p.add_argument('--probe-task-settings', action='store_true',
               help='check upstream application settings and task-local directories at normal desktop boot')
p.add_argument('--probe-source-context', action='store_true',
               help='check source-directory masks, project lookup and upstream relative document links')
p.add_argument('--probe-source-execute', action='store_true',
               help='check upstream ExeFile through the native compiler, including task-local and nested files')
p.add_argument('--probe-compiler-recovery', action='store_true',
               help='check statement rollback, continued execution and task-local compiler error recovery')
p.add_argument('--probe-compiler-context', action='store_true',
               help='check guest compiler contexts, option inheritance, restrictions and task isolation')
p.add_argument('--probe-run-file', action='store_true',
               help='check task-local LastFun and unchanged RunFile with raw fixed/variadic argument words')
p.add_argument('--probe-system-launch', action='store_true',
               help='check upstream Sys/SysFile and popup file execution at normal desktop boot')
p.add_argument('--probe-system-server', action='store_true',
               help='check the dedicated original ServerTaskCont loop and shared system namespace')
p.add_argument('--probe-user-command', action='store_true',
               help='check original ExeCmdLine/UserTaskCont through the ARM64 compiler ABI')
p.add_argument('--probe-user-startup', action='store_true',
               help='check original User/UserCmdLine, HomeSys hooks and independent graphical terminals')
p.add_argument('--probe-source-editor', action='store_true',
               help='check the full-screen native source editor controls, scanout and save/reopen path')
p.add_argument('--probe-user-text', action='store_true',
               help='check User formatted startup and XTalk input-filter delivery to graphical terminals')
p.add_argument('--probe-home-path', action='store_true',
               help='check shared HomeSet, home file execution and reboot persistence')
p.add_argument('--probe-registry', action='store_true',
               help='check DolDoc compiler input, registry trees and Once execution')
p.add_argument('--probe-home-once', action='store_true',
               help='check default include extension, real boot media and original home Once/tips')
p.add_argument('--probe-home-autostart', action='store_true',
               help='verify disk boot automatically sends the first-terminal HomeSys Once command')
p.add_argument('--probe-dvd-live', action='store_true',
               help='verify ISO/DVD boot enters the graphical live session and reports installer status')
p.add_argument('--probe-autocomplete', action='store_true',
               help='verify upstream autocomplete indexes a source corpus and stops cleanly')
p.add_argument('--probe-input-edges', action='store_true',
               help='check visible modifier typing and held-key release without the full workflow')
p.add_argument('--probe-source-exceptions', action='store_true',
               help='check catchable source/file errors, native cleanup and command/job recovery')
p.add_argument('--probe-winmgr', action='store_true',
               help='also try the unchanged WinMgr.ZC load with a short timeout after Win.ZC')
p.add_argument('--probe-grscreen', action='store_true',
               help='also load unchanged GrScreen.ZC after the WinMgr probe setup')
p.add_argument('--probe-fulltext', action='store_true',
               help='run an opt-in full GrUpdateTextBG framebuffer diagnostic')
p.add_argument('--probe-fulltext-visible', action='store_true',
               help='run the full-frame text diagnostic with framebuffer console visible')
p.add_argument('--probe-live-winmgr', action='store_true',
               help='start WinMgrTask and verify its first refresh and RLf_WINMGR marker')
p.add_argument('--probe-live-winmgr-wake', action='store_true',
               help='after the first refresh, leave the manager running through its timer wake')
p.add_argument('--probe-live-winmgr-ode', action='store_true',
               help='load MathODE before GrScreen and verify a live manager frame advances a task ODE')
p.add_argument('--probe-live-window-control', action='store_true',
               help='verify source CDC control rendering while WinMgrTask remains active')
p.add_argument('--probe-task-jobs', action='store_true', help='check CPU-0 callback jobs after loading pinned task headers')
p.add_argument('--probe-window-buttons', action='store_true', help='check live task-window title buttons in a disposable guest')
p.add_argument('--probe-graphics-palette', action='store_true',
               help='verify indexed graphics drawing follows the active ZealOS palette')
p.add_argument('--probe-grblot3', action='store_true',
               help='check the boot-loaded 3D bitmap blit path before window-demo stress tests')
p.add_argument('--probe-grline-cdc', action='store_true',
               help='check bounded CDC line and brush rasterization in isolation')
p.add_argument('--probe-screen-depth', action='store_true',
               help='check screen-coordinate depth isolation and per-window reset')
p.add_argument('--probe-sprite-arrow-symmetry', action='store_true',
               help='check the boot-loaded sprite arrow symmetry raster path')
p.add_argument('--probe-source', type=pathlib.Path, action='append',
               help='load a host source file from the disposable PCI source partition')
p.add_argument('--probe-source-call',
               help='call this no-argument integer function after each source probe and require 0x2a')
p.add_argument('--probe-source-kernel', action='store_true',
               help='load KernelA/B and KMathB before each source probe')
args = p.parse_args()
if args.usb_input:
    args.pci = True
    args.probe_gui_keyboard = True
if args.usb_with_virtio and not args.usb_input:
    p.error('--usb-with-virtio requires --usb-input')
if (args.usb_hubs or args.usb_hotplug) and not args.usb_input:
    p.error('--usb-hubs/--usb-hotplug require --usb-input')
usb_label = (f'{args.usb_input}-{args.usb_pointer}' +
             ('-virtio' if args.usb_with_virtio else '') +
             (f'-hubs{args.usb_hubs}' if args.usb_hubs else '') +
             ('-hotplug' if args.usb_hotplug else '')) if args.usb_input else None
if args.probe_gui_workflow:
    args.probe_gui_keyboard = True
if args.probe_gui_editor_text:
    args.probe_gui_keyboard = True
if (args.probe_stock_programs or args.probe_stock_interactive or args.probe_program_catalog or
        args.probe_doc_includes or args.probe_logic_program or args.probe_budget_program or
        args.probe_3d_apps):
    args.pci = True
    args.probe_gui_keyboard = True
if args.probe_lattice_controls:
    args.probe_gui_keyboard = True
if args.probe_gui_windowdemo_input:
    args.probe_gui_keyboard = True
if args.probe_input_edges:
    args.probe_gui_keyboard = True
if args.probe_module_override and not args.probe_module:
    p.error('--probe-module-override requires --probe-module')
if args.probe_fulltext_visible:
    args.probe_fulltext = True
if args.probe_source and not args.pci:
    p.error('--probe-source requires --pci')
if args.probe_graphics_palette and not args.pci:
    p.error('--probe-graphics-palette requires --pci')
if args.probe_grblot3 and not args.pci:
    p.error('--probe-grblot3 requires --pci')
if args.probe_screen_depth and not args.pci:
    p.error('--probe-screen-depth requires --pci')
if args.probe_sprite_arrow_symmetry and not args.pci:
    p.error('--probe-sprite-arrow-symmetry requires --pci')
if args.probe_home_autostart and not args.pci:
    p.error('--probe-home-autostart requires --pci')
if args.probe_source_editor and not args.pci:
    p.error('--probe-source-editor requires --pci')
if args.probe_autocomplete and not args.pci:
    p.error('--probe-autocomplete requires --pci')
if args.probe_gui_terminal and (
        not args.pci or args.probe_module != '/System/DolDoc/DocRecalc.ZC'):
    p.error('--probe-gui-terminal requires --pci --probe-module /System/DolDoc/DocRecalc.ZC')
if args.probe_gui_keyboard and not args.pci:
    p.error('--probe-gui-keyboard requires --pci')
if args.probe_gui_editor_text and not args.pci:
    p.error('--probe-gui-editor-text requires --pci')
if args.probe_lattice_controls and not args.pci:
    p.error('--probe-lattice-controls requires --pci')
if args.probe_gui_windowdemo_input and not args.pci:
    p.error('--probe-gui-windowdemo-input requires --pci')
if args.probe_grscreen and not args.probe_winmgr:
    p.error('--probe-grscreen requires --probe-winmgr')
if args.probe_fulltext and not args.probe_grscreen:
    p.error('--probe-fulltext requires --probe-grscreen')
if args.probe_live_winmgr and not args.probe_grscreen:
    p.error('--probe-live-winmgr requires --probe-grscreen')
if args.probe_live_winmgr_wake and not args.probe_live_winmgr:
    p.error('--probe-live-winmgr-wake requires --probe-live-winmgr')
if args.probe_live_winmgr_ode and not (
        args.probe_module == '/System/Win.ZC' and args.probe_winmgr and
        args.probe_grscreen and args.probe_live_winmgr):
    p.error('--probe-live-winmgr-ode requires --probe-module /System/Win.ZC '
            '--probe-winmgr --probe-grscreen --probe-live-winmgr')
if args.probe_live_window_control and not (
        args.probe_module == '/System/Win.ZC' and args.probe_winmgr and
        args.probe_grscreen and args.probe_live_winmgr):
    p.error('--probe-live-window-control requires --probe-module /System/Win.ZC '
            '--probe-winmgr --probe-grscreen --probe-live-winmgr')
if args.probe_dvd_live and args.pci:
    p.error('--probe-dvd-live requires ISO boot, not --pci')
log_path = ROOT / 'build' / (f'check-usb-{usb_label}{"-budget" if args.probe_budget_program else "-logic" if args.probe_logic_program else "-program-catalog" if args.probe_program_catalog else "-stock-interactive" if args.probe_stock_interactive else "-stock-programs" if args.probe_stock_programs else "-lattice-controls" if args.probe_lattice_controls else "-windowdemo-input" if args.probe_gui_windowdemo_input else "-home-once" if args.probe_home_once else "-input-edges" if args.probe_input_edges else "-registry" if args.probe_registry else "-home-path" if args.probe_home_path else "-user-text" if args.probe_user_text else "-user-startup" if args.probe_user_startup else ""}.log' if args.usb_input else
                             'check-budget-pci.log' if args.probe_budget_program else
                             'check-3d-apps-pci.log' if args.probe_3d_apps else
                             'check-logic-pci.log' if args.probe_logic_program else
                             'check-doc-includes-pci.log' if args.probe_doc_includes else
                             'check-program-catalog-pci.log' if args.probe_program_catalog else
                             'check-stock-interactive-pci.log' if args.probe_stock_interactive else
                             'check-stock-programs-pci.log' if args.probe_stock_programs else
                             'check-grblot3-pci.log' if args.probe_grblot3 else
                             'check-sprite-arrow-symmetry-pci.log' if args.probe_sprite_arrow_symmetry else
                             'check-home-once-pci.log' if args.probe_home_once and args.pci else
                             'check-home-once-iso.log' if args.probe_home_once else
                             'check-dvd-live.log' if args.probe_dvd_live else
                             'check-source-editor-pci.log' if args.probe_source_editor else
                             'check-home-autostart-pci.log' if args.probe_home_autostart else
                             'check-autocomplete-pci.log' if args.probe_autocomplete else
                             'check-input-edges-pci.log' if args.probe_input_edges else
                             'check-registry-pci.log' if args.probe_registry and args.pci else
                             'check-registry-iso.log' if args.probe_registry else
                             'check-home-path-pci.log' if args.probe_home_path and args.pci else
                             'check-home-path-iso.log' if args.probe_home_path else
                             'check-user-text-pci.log' if args.probe_user_text else
                             'check-user-startup-pci.log' if args.probe_user_startup and args.pci else
                             'check-user-startup-iso.log' if args.probe_user_startup else
                             'check-gui-editor-text-pci.log' if args.probe_gui_editor_text else
                             'check-gui-workflow.log' if args.probe_gui_workflow else
                             'check-lattice-controls-pci.log' if args.probe_lattice_controls else
                             'check-live-window-control.log' if args.probe_live_window_control else
                             'check-screen-depth-pci.log' if args.probe_screen_depth else
                             'check-graphics-palette.log' if args.probe_graphics_palette else
                             'check-window-buttons.log' if args.probe_window_buttons else
                             'check-winmgr-live.log' if args.probe_live_winmgr else
                             'check-module-probe.log' if args.probe_module else
                             'check-upstream-probe.log' if args.probe_upstream else
                             'check-compat-pci.log' if args.pci else 'check-compat.log')
with tempfile.TemporaryDirectory(prefix='zeal-compat-') as tmp, log_path.open('wb') as log:
    tmp = pathlib.Path(tmp)
    fw = os.environ.get('FW_CODE', '/opt/homebrew/share/qemu/edk2-aarch64-code.fd')
    shutil.copyfile(os.environ.get('FW_VARS_IN', '/opt/homebrew/share/qemu/edk2-arm-vars.fd'), tmp/'vars.fd')
    platform_args = shlex.split(os.environ.get('QEMU_VIRT', '-machine virt,accel=hvf,gic-version=3 -cpu host -m 512 -device ramfb -device virtio-keyboard-device -device virtio-tablet-device'))
    if args.usb_input:
        if not args.usb_with_virtio:
            filtered = []
            i = 0
            while i < len(platform_args):
                if (platform_args[i] == '-device' and i + 1 < len(platform_args) and
                        platform_args[i+1].split(',')[0] in (
                            'virtio-keyboard-device', 'virtio-tablet-device',
                            'virtio-keyboard-pci', 'virtio-tablet-pci')):
                    i += 2
                else:
                    filtered.append(platform_args[i]); i += 1
            platform_args = filtered
        model = 'nec-usb-xhci' if args.usb_input == 'nec' else 'qemu-xhci'
        platform_args += ['-device', model+',id=usb']
        for tier in range(args.usb_hubs):
            platform_args += ['-device', f'usb-hub,bus=usb.0,port={"1."*tier}1,id=usb-hub{tier}']
        usb_prefix = '1.' * args.usb_hubs
        platform_args += ['-device', f'usb-kbd,bus=usb.0,port={usb_prefix}1,id=usb-kbd',
                          '-device', f'usb-mouse,bus=usb.0,port={usb_prefix}2,id=usb-mouse']
        if args.usb_pointer == 'tablet':
            platform_args += ['-device', f'usb-tablet,bus=usb.0,port={usb_prefix}3,id=usb-tablet']
    cmd = [os.environ.get('QEMU', 'qemu-system-aarch64')] + platform_args + ['-drive', f'if=pflash,unit=0,format=raw,file={fw},readonly=on', '-drive', f'if=pflash,unit=1,format=raw,file={tmp}/vars.fd', '-serial', 'stdio', '-monitor', 'none', '-display', 'none']
    qmp_path = tmp/'qmp.sock'
    # Both boot shapes have a VirtIO keyboard. Focused guest children receive
    # device input; UART remains independent for the serial command shell.
    cmd += ['-qmp', f'unix:{qmp_path},server=on,wait=off']
    if args.pci:
        shutil.copyfile(ROOT/'build/disk.img', tmp/'disk.img')
        # A torn shadow-bank superblock must never supersede the valid A bank.
        if not args.probe_source and not args.probe_upstream:
            with (tmp/'disk.img').open('r+b') as f:
                f.seek(196575 * 512)
                partial = bytearray(512)
                partial[:8] = b'ZCSRC002'
                struct.pack_into('<III', partial, 8, 2, 65536, 999)
                f.write(partial)  # CRC deliberately absent
                f.write(b'partial-copy' + bytes(512 - len(b'partial-copy')))
        if args.probe_source:
            for i, source in enumerate(args.probe_source):
                subprocess.run(['python3', str(ROOT/'scripts/source-volume.py'), 'put',
                                str(tmp/'disk.img'), str(source), f'Probe/Source{i}.ZC'],
                               check=True, stdout=subprocess.DEVNULL)
        if args.probe_home_autostart:
            subprocess.run(['python3', str(ROOT/'scripts/source-volume.py'), 'put',
                            str(tmp/'disk.img'), str(ROOT/'tests/compat/HomeAutoOnce.ZC'),
                            'Home/Once.ZC'], check=True, stdout=subprocess.DEVNULL)
            subprocess.run(['python3', str(ROOT/'scripts/source-volume.py'), 'put',
                            str(tmp/'disk.img'), str(ROOT/'tests/compat/SplashWindow.ZC'),
                            'Tests/SplashWindow.ZC'], check=True, stdout=subprocess.DEVNULL)
        cmd += ['-drive', f'if=none,format=raw,file={tmp}/disk.img,id=esp', '-device', 'virtio-blk-pci,drive=esp,bootindex=0']
    else:
        redsea = bytearray(65536)
        if args.probe_module in ('/System/DolDoc/DocPlain.ZC',
                                 '/System/DolDoc/DocFile.ZC'):
            # Exercise FileWrite's legacy RedSea fallback on a blank,
            # formatted root volume, matching rs_fmt_host's on-disk layout.
            redsea[3] = 0x88
            struct.pack_into('<5q', redsea, 8, 0, 128, 2, 1, 7)
            redsea[510:512] = b'\x55\xaa'
            redsea[512] = 0x07
        (tmp/'redsea.img').write_bytes(redsea)
        cmd += ['-drive', f'if=none,format=raw,file={tmp}/redsea.img,id=rs', '-device', 'virtio-blk-device,drive=rs', '-drive', f'if=none,file={ROOT}/build/zealos-aarch64-hello.iso,id=cd,media=cdrom,readonly=on', '-device', 'virtio-scsi-pci,id=scsi', '-device', 'scsi-cd,drive=cd,bootindex=0']
    guest = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, bufsize=0)
    buf = bytearray()
    def until(marker, timeout=120):
        start = time.monotonic()
        while marker not in buf:
            if time.monotonic()-start > timeout:
                raise RuntimeError(f'timeout waiting for {marker!r}\n'+buf[-4000:].decode(errors='replace'))
            if select.select([guest.stdout], [], [], .2)[0]:
                data = os.read(guest.stdout.fileno(), 65536)
                if not data: raise RuntimeError('guest exited')
                buf.extend(data); log.write(data); log.flush()
                fault_start = buf.find(b'sync: ESR=')
                if (fault_start >= 0 and b'\n' in buf[fault_start:] and
                        marker not in buf):
                    raise RuntimeError('guest synchronous fault\n'+buf[-2000:].decode(errors='replace'))
        end = buf.index(marker)+len(marker)
        result = bytes(buf[:end]); del buf[:end]
        return result.decode(errors='replace')
    def command(line, expected, timeout=120):
        # A full DolDoc refresh can keep the cooperative guest away from its
        # UART for longer than a host pipe write. Wait for each small echoed
        # byte before sending more, so long diagnostics cannot overrun the
        # emulated PL011 receive FIFO while several windows are painting.
        result = ''
        encoded = line.encode()
        for start in range(len(encoded)):
            chunk = encoded[start:start+1]
            guest.stdin.write(chunk); guest.stdin.flush()
            result += until(chunk, timeout=timeout)
        guest.stdin.write(b'\r'); guest.stdin.flush()
        result += until(b'\n> ', timeout=timeout)
        if expected not in result: raise RuntimeError(f'{line}: missing {expected!r}\n{result}')
        print(f'PASS {line}: {expected}', flush=True)
        return result
    def qmp_command(execute, arguments=None):
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            sock.settimeout(5)
            deadline = time.monotonic() + 10
            while True:
                try:
                    sock.connect(str(qmp_path))
                    break
                except (FileNotFoundError, ConnectionRefusedError):
                    if time.monotonic() >= deadline:
                        raise RuntimeError('QEMU QMP socket did not become ready')
                    time.sleep(0.05)
            stream = sock.makefile('rwb')
            greeting = json.loads(stream.readline())
            if 'QMP' not in greeting:
                raise RuntimeError('invalid QMP greeting: '+repr(greeting))
            stream.write(b'{"execute":"qmp_capabilities"}\r\n')
            stream.flush()
            if 'return' not in json.loads(stream.readline()):
                raise RuntimeError('QMP capability negotiation failed')
            request = {'execute': execute}
            if arguments is not None:
                request['arguments'] = arguments
            stream.write((json.dumps(request)+'\r\n').encode())
            stream.flush()
            response = json.loads(stream.readline())
            while 'event' in response:
                response = json.loads(stream.readline())
            if 'return' not in response:
                raise RuntimeError(f'QMP {execute} failed: '+repr(response))
            stream.close()
            return response['return']
        finally:
            sock.close()
    def qmp_key_edges(qcodes, down):
        qmp_command('input-send-event', {'events': [
            {'type': 'key', 'data': {'down': down,
             'key': {'type': 'qcode', 'data': qcode}}} for qcode in qcodes]})
    def qmp_send_keys(qcodes):
        # Send chords as ordered edges, with enough spacing for the guest to
        # sample each transition under load. Grouping Shift+key in one QMP
        # request or releasing Shift too quickly can leave the modifier stuck
        # and corrupt the next mixed-case command.
        for qcode in qcodes:
            qmp_key_edges([qcode], True)
            time.sleep(0.1)
        for qcode in reversed(qcodes):
            qmp_key_edges([qcode], False)
            time.sleep(0.1)
        for modifier in ('shift', 'ctrl', 'alt'):
            if modifier in qcodes:
                qmp_key_edges([modifier], False)
                time.sleep(0.1)
    def graphical_path(name):
        # Independent program/workflow gates may run concurrently. Keep
        # their QMP screendumps separate so one VM cannot satisfy another's
        # visible-pixel or OCR assertions.
        mode = ('budget-' if args.probe_budget_program else
                '3d-apps-' if args.probe_3d_apps else
                'doc-includes-' if args.probe_doc_includes else
                'logic-' if args.probe_logic_program else
                'interactive-' if args.probe_stock_interactive else
                'stock-' if args.probe_stock_programs else
                'workflow-' if args.probe_gui_workflow else
                'catalog-' if args.probe_program_catalog else '')
        screen_name = f'usb-{usb_label}-{mode}{name}' if args.usb_input else mode+name
        return ROOT/'build'/f'gui-{screen_name}.ppm'
    def graphical_screen(name, foreground_threshold=240000,
                         scanline_offset=1):
        # Inspect actual scanout, rather than treating successful command
        # assembly as proof that the user could see characters while typing.
        path = graphical_path(name)
        qmp_command('screendump', {'filename': str(path)})
        ppm = path.read_bytes()
        header = re.match(rb'P6\s+(\d+)\s+(\d+)\s+255\s', ppm)
        if not header:
            raise RuntimeError(f'invalid framebuffer screenshot: {path}')
        width, height = map(int, header.groups())
        pixels = ppm[header.end():]
        if len(pixels) != width * height * 3:
            raise RuntimeError(f'incomplete framebuffer screenshot: {path}')
        glyphs = re.findall(r'/\*.*?\*/\s*\{([^}]+)\}',
                            (ROOT/'src/fb_font.h').read_text(), re.S)[:96]
        font = {tuple(int(v.strip(), 0) for v in glyph.split(',')): chr(i+32)
                for i, glyph in enumerate(glyphs)}
        # DolDoc menu values underline the last scanline of each glyph.
        # Recognize that real output as well as ordinary terminal text.
        font.update({bits[:-1] + (255,): char for bits, char in tuple(font.items())})
        rows = []
        # The source planar font starts one scanline below the text cell;
        # direct pixel graphics begin at the requested scanline.
        for y in range(scanline_offset, height-7, 8):
            row = ''
            for x in range(0, width-7, 8):
                bits = tuple(sum(0x80 >> col for col in range(8)
                                 if (pixels[((y+r)*width+x+col)*3] * 299 +
                                     pixels[((y+r)*width+x+col)*3+1] * 587 +
                                     pixels[((y+r)*width+x+col)*3+2] * 114) > foreground_threshold)
                             for r in range(8))
                row += font.get(bits, font.get(tuple(v ^ 255 for v in bits), '?'))
            rows.append(row.rstrip())
        text = '\n'.join(rows)
        path.with_suffix('.txt').write_text(text+'\n')
        return width, height, pixels, text
    def require_graphical_text(name, expected, timeout=5,
                               foreground_threshold=240000,
                               scanline_offset=1):
        deadline = time.monotonic() + timeout
        while True:
            screen = graphical_screen(name, foreground_threshold,
                                      scanline_offset)
            if expected in screen[3]:
                break
            if time.monotonic() >= deadline:
                guest.stdin.write(b'zstatus\r'); guest.stdin.flush()
                try:
                    diagnostic = until(b'\n> ', timeout=5)
                except RuntimeError as error:
                    diagnostic = str(error)
                print('GRAPHICAL FAILURE STATUS\n'+diagnostic, flush=True)
                raise RuntimeError(f'graphical scanout is missing {expected!r}; '
                                   f'see {graphical_path(name)} and .txt\n'+screen[3])
            time.sleep(0.1)
        print(f'PASS visible graphical text: {expected!r}', flush=True)
        return screen
    def require_graphical_prompt_text(name, expected, timeout=5):
        deadline = time.monotonic() + timeout
        pattern = re.compile(r'>[ \t]+' + re.escape(expected))
        while True:
            screen = graphical_screen(name)
            if any(pattern.search(line) for line in screen[3].splitlines()):
                print(f'PASS visible graphical prompt text: {expected!r}', flush=True)
                return screen
            if time.monotonic() >= deadline:
                guest.stdin.write(b'zstatus\r'); guest.stdin.flush()
                try:
                    diagnostic = until(b'\n> ', timeout=5)
                except RuntimeError as error:
                    diagnostic = str(error)
                print('GRAPHICAL FAILURE STATUS\n'+diagnostic, flush=True)
                raise RuntimeError(f'graphical prompt is missing {expected!r}; '
                                   f'see {graphical_path(name)} and .txt\n'+screen[3])
            time.sleep(0.1)
    def type_graphical_prompt_line(name, text, attempts=3, prompt_required=True):
        # After focus changes, a guest may miss one early key edge even when
        # QMP accepted the event. Clear and verify the guest's input buffer
        # before each attempt so a retry never appends to a partial command.
        # The rendered desktop contains two overlapping terminal columns, so
        # line-based screenshot matching cannot reliably identify an empty
        # prompt after a window demo has returned.
        for attempt in range(attempts):
            qmp_send_key('down')
            clear_deadline = time.monotonic() + 3
            while True:
                state = command('zcall BootstrapGuiTerminalInputDiag',
                                'zc => 0x')
                match = re.search(r'zc => 0x([0-9a-fA-F]+)', state)
                input_length = int(match.group(1), 16) & 255 if match else -1
                if input_length == 0:
                    break
                if time.monotonic() >= clear_deadline:
                    screen = graphical_screen(name+'-empty-prompt')
                    raise RuntimeError('could not clear the graphical prompt before typing; '
                                       f'see {graphical_path(name + "-empty-prompt")}\n'
                                       f'input length={input_length}\n' + screen[3])
                time.sleep(0.1)
            qmp_send_text(text)
            try:
                if not prompt_required:
                    screen = graphical_screen(name)
                    state = command('zc Print("gui_input_length=%d\\n", '
                                    'BootstrapGuiTerminalInputDiag() & 255);',
                                    'gui_input_length=')
                    match = re.search(r'gui_input_length=(\d+)', state)
                    if match and int(match.group(1)) == len(text) and text in screen[3]:
                        print(f'PASS visible graphical terminal input buffer: {text!r}',
                              flush=True)
                        return screen
                return require_graphical_prompt_text(name, text, timeout=8)
            except RuntimeError:
                if attempt + 1 == attempts:
                    raise
        raise RuntimeError(f'could not type graphical prompt line {text!r}')
    def run_graphical_terminal_command(name, source, expected, timeout=12):
        # Keep the long GUI workflow deterministic: wait for the whole command
        # to be visible before Return, then require one new result line.
        before_count = graphical_screen(name+'-before')[3].count(expected)
        type_graphical_prompt_line(name+'-command', source)
        qmp_send_key('ret')
        deadline = time.monotonic() + timeout
        while True:
            screen = graphical_screen(name+'-result')
            if screen[3].count(expected) > before_count:
                return screen
            if time.monotonic() >= deadline:
                raise RuntimeError(f'graphical terminal command {source!r} did not produce '
                                   f'{expected!r}; see {graphical_path(name+"-result")}\n' +
                                   screen[3])
            time.sleep(0.1)
    def require_graphical_text_absent(name, unexpected, timeout=5):
        deadline = time.monotonic() + timeout
        while True:
            screen = graphical_screen(name)
            if unexpected not in screen[3]:
                print(f'PASS graphical text cleared: {unexpected!r}', flush=True)
                return screen
            if time.monotonic() >= deadline:
                raise RuntimeError(f'graphical scanout still contains {unexpected!r}; '
                                   f'see {graphical_path(name)} and .txt\n'+screen[3])
            time.sleep(0.1)
    def rgb_pixels_in(screen, rgb, bounds):
        width, height, pixels, _ = screen
        left, top, right, bottom = bounds
        left, right = max(0, left), min(width, right)
        top, bottom = max(0, top), min(height, bottom)
        color = bytes(rgb)
        return sum(pixels[(y*width+x)*3:(y*width+x)*3+3] == color
                   for y in range(top, bottom) for x in range(left, right))
    def rgb_coordinates_in(screen, rgb, bounds):
        width, height, pixels, _ = screen
        left, top, right, bottom = bounds
        left, right = max(0, left), min(width, right)
        top, bottom = max(0, top), min(height, bottom)
        color = bytes(rgb)
        return {(x, y) for y in range(top, bottom) for x in range(left, right)
                if pixels[(y*width+x)*3:(y*width+x)*3+3] == color}
    def graphical_ascii_glyph_matches(screen, x, y, ch, rgb):
        width, height, pixels, _ = screen
        glyphs = re.findall(r'/\*.*?\*/\s*\{([^}]+)\}',
                            (ROOT/'src/fb_font.h').read_text(), re.S)[:96]
        rows = [int(value.strip(), 0)
                for value in glyphs[ord(ch) - 32].split(',')]
        color = bytes(rgb)
        # GrPutChar's font starts one scanline below the menu row origin.
        for row, bits in enumerate(rows):
            for col in range(8):
                offset = ((y + 1 + row) * width + x + col) * 3
                if offset < 0 or offset + 3 > len(pixels):
                    return False
                drawn = pixels[offset:offset + 3] == color
                if drawn != bool(bits & (0x80 >> col)):
                    return False
        return True
    def splash_label_pixel_counts(screen):
        width, height, _, _ = screen
        cx, cy = width // 2, height // 2
        gray = (170, 170, 170)  # LTGRAY backing rectangles from upstream ZSplash.
        title = rgb_pixels_in(screen, gray,
                              (cx-130, cy-92, cx+130, cy-68))
        motto = rgb_pixels_in(screen, gray,
                              (cx-130, cy+68, cx+130, cy+92))
        return title, motto
    def splash_center_mesh_pixels(screen):
        width, height, pixels, _ = screen
        cx, cy = width // 2, height // 2
        count = 0
        for y in range(max(0, cy-64), min(height, cy+65)):
            for x in range(max(0, cx-64), min(width, cx+65)):
                i = (y*width+x)*3
                red, green, blue = pixels[i:i+3]
                if blue >= 80 and blue > red * 1.5 and blue > green * 1.25:
                    count += 1
        return count
    def is_graphical_splash_visible(screen):
        title, motto = splash_label_pixel_counts(screen)
        # ZSplash uses GrPrint (black glyphs on LTGRAY), so its lettering is
        # graphical pixels rather than the white DolDoc text OCR recognizes.
        return title >= 200 and motto >= 600
    def require_graphical_splash(name, timeout=20):
        deadline = time.monotonic() + timeout
        while True:
            screen = graphical_screen(name)
            if is_graphical_splash_visible(screen):
                break
            if time.monotonic() >= deadline:
                title, motto = splash_label_pixel_counts(screen)
                status = command('zstatus', 'keyboard_focus=')
                raise RuntimeError('animated graphical splash pixels are missing '
                                   f'(title={title}, motto={motto}):\n{status}\n'
                                   f'see {graphical_path(name)} and .txt\n{screen[3]}')
            time.sleep(0.1)
        time.sleep(0.2)
        next_screen = graphical_screen(name + '-motion')
        width, height, pixels, _ = screen
        next_pixels = next_screen[2]
        cx, cy = width // 2, height // 2
        # The source splash animates in the left half of this framebuffer;
        # the right half is the graphical terminal. Compare the whole splash
        # pane so the off-center rotated mesh is included.
        changed = 0
        for y in range(height):
            for x in range(cx):
                i = (y*width+x)*3
                if pixels[i:i+3] != next_pixels[i:i+3]:
                    changed += 1
        if changed < 10:
            raise RuntimeError('splash labels are visible but the animated sprite '
                               f'did not change between frames ({changed} pixels); '
                               f'see {graphical_path(name)} and .txt')
        title, motto = splash_label_pixel_counts(next_screen)
        if title < 200 or motto < 600:
            raise RuntimeError('splash labels disappeared during animation capture '
                               f'(title={title}, motto={motto})')
        center_mesh = splash_center_mesh_pixels(next_screen)
        if center_mesh < 40:
            raise RuntimeError('animated splash mesh is not centered between its '
                               f'labels (center blue pixels={center_mesh}); '
                               f'see {graphical_path(name + "-motion")} and .txt')
        print(f'PASS upstream graphical splash labels and animation ({changed} changed pixels)',
              flush=True)
        print(f'PASS splash mesh rendered in its centered logo area '
              f'({center_mesh} blue pixels)', flush=True)
        return next_screen
    def require_graphical_splash_absent(name, timeout=5):
        deadline = time.monotonic() + timeout
        while True:
            screen = graphical_screen(name)
            if not is_graphical_splash_visible(screen):
                print('PASS splash graphic cleared after dismissal', flush=True)
                return screen
            if time.monotonic() >= deadline:
                title, motto = splash_label_pixel_counts(screen)
                raise RuntimeError('graphical splash remains visible after dismissal '
                                   f'(title={title}, motto={motto}); '
                                   f'see {graphical_path(name)}')
            time.sleep(0.1)
    def dismiss_home_splash_if_visible():
        screen = graphical_screen('before-terminal-focus')
        if is_graphical_splash_visible(screen):
            qmp_send_key('esc')
            try:
                require_graphical_splash_absent('boot-splash-dismissed')
            except RuntimeError:
                status_after = command('zstatus', 'keyboard_focus=')
                print('BOOT SPLASH POST-DISMISS STATUS\n'+status_after, flush=True)
                raise
            return
        # The popup can already own focus before its static DolDoc is visible
        # in scanout. Home startup runs PopUp synchronously, so the command
        # line may remain blocked with the transient Server task focused.
        # Dismiss that startup splash by its focus owner in this case.
        status = command('zstatus', 'keyboard_focus=')
        if 'keyboard_focus=Server' in status:
            qmp_send_key('esc')
            time.sleep(0.2)
    def white_pixels(screen, x, y, width, height):
        stride, _, pixels, _ = screen
        return sum(pixels[((y+r)*stride+x+c)*3:((y+r)*stride+x+c)*3+3]
                   == b'\xff\xff\xff' for r in range(height) for c in range(width))
    def require_graphical_caret(screen, line):
        deadline = time.monotonic() + 6
        pattern = (re.compile(r'>[ \t]+' + re.escape(line[2:]))
                   if line.startswith('> ') else re.compile(re.escape(line)))
        while True:
            for row, text in enumerate(screen[3].splitlines()):
                # Repeated commands may also appear in history, another
                # terminal or a title. Inspect every occurrence for the caret.
                for match in pattern.finditer(text):
                    col = match.end()
                    if white_pixels(screen, col*8, row*8, 8, 8) >= 8:
                        print('PASS visible graphical insertion caret', flush=True)
                        return
            if time.monotonic() >= deadline:
                break
            time.sleep(0.1)
            screen = graphical_screen('typing-before-return')
        raise RuntimeError('graphical input has no visible insertion caret; '
                           f'see {graphical_path("typing-before-return")}')
    pointer_xy = [400, 300]
    def move_graphical_pointer(x, y):
        if args.usb_input and args.usb_pointer == 'mouse':
            qmp_command('input-send-event', {'events': [
                {'type': 'rel', 'data': {'axis': 'x', 'value': x-pointer_xy[0]}},
                {'type': 'rel', 'data': {'axis': 'y', 'value': y-pointer_xy[1]}}]})
            pointer_xy[:] = x, y
            return
        qmp_command('input-send-event', {'events': [
            {'type': 'abs', 'data': {'axis': 'x', 'value': round(x*32767/799)}},
            {'type': 'abs', 'data': {'axis': 'y', 'value': round(y*32767/599)}}]})
    def graphical_mouse_button(button, down):
        qmp_command('input-send-event', {'events': [
            {'type': 'btn', 'data': {'down': down, 'button': button}}]})
    menu_root_x_cache = {}
    def graphical_menu_root_x(root_name):
        # Ask the live menu for its root position. This keeps pointer tests
        # correct when grouped menu labels or submenu widths change.
        if root_name not in menu_root_x_cache:
            function = 'CompatGuiMenuRootX' + re.sub(r'[^A-Za-z0-9]', '', root_name)
            command(
                f'zc I64 {function}(){{CMenuEntry*e;I64 x=0;'
                'for(e=bootstrap_gui_terminal_task->cur_menu->sub;'
                f'e&&StrCompare(e->name,"{root_name}");e=e->next)'
                'x+=MenuEntryWidth(e)*FONT_WIDTH;return x;}',
                'zc: executed')
            result = command(f'zcall {function}', 'zc => 0x')
            match = re.search(r'zc => 0x([0-9a-fA-F]+)', result)
            if not match:
                raise RuntimeError(f'could not locate the {root_name} menu root:\n' +
                                   result)
            menu_root_x_cache[root_name] = int(match.group(1), 16)
        return menu_root_x_cache[root_name]
    def select_os_menu_row(name, root_name, row_index):
        # Press on the requested menu root, drag to a submenu row, and
        # release, matching the native WinMgr gesture instead of calling the
        # action bridge directly.
        root_x = graphical_menu_root_x(root_name)
        move_graphical_pointer(root_x + 4, 4)
        deadline = time.monotonic() + 5
        screen = graphical_screen(name+'-open')
        while time.monotonic() < deadline:
            if (rgb_pixels_in(screen, (0, 0, 170),
                              (root_x, 0, min(root_x+240, 800), 8)) >= 100 and
                    rgb_pixels_in(screen, (0, 0, 170),
                                  (root_x, 8, min(root_x+240, 800), 100)) >= 500):
                break
            time.sleep(0.1)
            screen = graphical_screen(name+'-open')
        else:
            raise RuntimeError('OS menu did not open for '+name+':\n'+screen[3])
        qmp_command('input-send-event', {'events': [
            {'type': 'btn', 'data': {'down': True, 'button': 'left'}}]})
        time.sleep(0.2)
        press_status = command('zstatus', 'pointer_state: ready=1')
        press_match = re.search(
            r'pointer_state: ready=1 pointer_xy=(\d+)/(\d+) buttons=(\d+)',
            press_status)
        if (not press_match or int(press_match.group(3)) != 1 or
                abs(int(press_match.group(1)) - (root_x + 4)) > 1 or
                abs(int(press_match.group(2)) - 4) > 1):
            qmp_command('input-send-event', {'events': [
                {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
            raise RuntimeError('OS menu press did not reach the guest for '+name+':\n'+
                               press_status)
        row_y = 12 + row_index * 8
        move_graphical_pointer(root_x + 4, row_y)
        time.sleep(0.2)
        screen = graphical_screen(name+'-held')
        held_status = command('zstatus', 'pointer_state: ready=1')
        held_match = re.search(
            r'pointer_state: ready=1 pointer_xy=(\d+)/(\d+) buttons=(\d+)',
            held_status)
        if (not held_match or int(held_match.group(3)) != 1 or
                abs(int(held_match.group(1)) - (root_x + 4)) > 1 or
                abs(int(held_match.group(2)) - row_y) > 1):
            qmp_command('input-send-event', {'events': [
                {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
            raise RuntimeError('OS menu drag did not reach '+name+':\n'+
                               held_status+screen[3])
        if rgb_pixels_in(screen, (255, 255, 85),
                         (root_x, row_y-4, min(root_x+240, 800), row_y+4)) < 32:
            qmp_command('input-send-event', {'events': [
                {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
            raise RuntimeError('OS menu did not highlight '+name+':\n'+
                               held_status+screen[3])
        qmp_command('input-send-event', {'events': [
            {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
    def require_graphical_pointer():
        # A newly attached tablet starts at zero, while QEMU can retain its
        # last host absolute position. Force an actual axis change before
        # checking pixels, as physical mouse motion would after reconnect.
        if not (args.usb_input and args.usb_pointer == 'mouse'):
            move_graphical_pointer(600, 440)
            time.sleep(0.1)
        move_graphical_pointer(640, 480)
        deadline = time.monotonic() + 5
        while True:
            screen = graphical_screen('mouse-pointer')
            if white_pixels(screen, 638, 478, 20, 22) >= 15:
                print('PASS visible graphical mouse pointer', flush=True)
                return
            if time.monotonic() >= deadline:
                raise RuntimeError('tablet motion has no visible mouse pointer; '
                                   f'see {graphical_path("mouse-pointer")}')
            time.sleep(0.1)
    def qmp_send_key(qcode):
        qmp_send_keys([qcode])
        # Separate navigation-probe keystrokes and allow guest polling after
        # the explicit release; the typing gate accounts for the hold time.
        time.sleep(0.1)
    def focus_graphical_terminal():
        dismiss_home_splash_if_visible()
        location = command('zc Print("gui_click=%d,%d\\n", '
            '(bootstrap_gui_terminal_task->pix_left+bootstrap_gui_terminal_task->pix_right)/2, '
            '(bootstrap_gui_terminal_task->pix_top+bootstrap_gui_terminal_task->pix_bottom)/2);',
            'gui_click=')
        point = re.search(r'gui_click=(-?\d+),(-?\d+)', location)
        if not point:
            raise RuntimeError('could not read graphical terminal bounds:\n'+location)
        status = ''
        for _ in range(5):
            move_graphical_pointer(int(point.group(1)), int(point.group(2)))
            for down in (True, False):
                qmp_command('input-send-event', {'events': [
                    {'type': 'btn', 'data': {'down': down, 'button': 'left'}}]})
                time.sleep(0.15)
            status = command('zc Print("primary_terminal_focus=%d\\n", '
                             'sys_focus_task==bootstrap_gui_terminal_task);',
                             'primary_terminal_focus=')
            if 'primary_terminal_focus=1' in status:
                command('zc BootstrapGuiTerminalDocBottom('
                        'bootstrap_gui_terminal_task->put_doc);', 'zc: executed')
                print('PASS graphical terminal acquired keyboard focus', flush=True)
                return
            # Home startup can launch its splash popup just after the first
            # focus check. If that late task steals focus, dismiss it before
            # the next click rather than repeatedly clicking the popup.
            zstatus = command('zstatus', 'keyboard_focus=')
            if 'keyboard_focus=Server' in zstatus:
                qmp_send_key('esc')
                time.sleep(0.2)
            time.sleep(0.1)
        raise RuntimeError('primary graphical terminal did not acquire keyboard focus after five clicks:\n'+status)
    def graphical_task_rect(task_expr):
        result = command(f'zc Print("gterm_rect=%d,%d,%d,%d\\n", '
            f'{task_expr}->win_left, {task_expr}->win_right, '
            f'{task_expr}->win_top, {task_expr}->win_bottom);', 'gterm_rect=')
        match = re.search(r'gterm_rect=(-?\d+),(-?\d+),(-?\d+),(-?\d+)', result)
        if not match:
            raise RuntimeError(f'could not read {task_expr} geometry:\n' + result)
        return tuple(map(int, match.groups()))
    def probe_graphical_task_resize(task_expr, label):
        # Check both the ARM64 bootstrap terminal and the independent terminal
        # created by upstream User(). The small TERM/EOF autocomplete popup is
        # DOCF_SIZE_MIN and intentionally follows its content; these regular
        # terminal windows must retain their bounds after release, settling,
        # and another command.
        command(f'zc WinToTop({task_expr});', 'zc: executed')
        original = graphical_task_rect(task_expr)
        left, right, top, bottom = original
        dx = min(4, 98 - right)
        # A window already at the right screen edge can still be tested by
        # shrinking it from the lower-right corner.
        if dx <= 0:
            dx = -min(4, right - left - 1)
        # The test display can start with the terminal nearly full-height.
        # In that case shrink from the lower-right corner instead of trying
        # to grow past the visible screen edge.
        dy = 2 if bottom <= 71 else -2
        if dx == 0 or top >= bottom + dy:
            raise RuntimeError(f'{label} has no resize room at {original}')
        start_x = (right + 1) * 8 + 4
        start_y = (bottom + 1) * 8 + 4
        try:
            move_graphical_pointer(start_x, start_y)
            qmp_command('input-send-event', {'events': [
                {'type': 'btn', 'data': {'down': True, 'button': 'left'}}]})
            time.sleep(0.15)
            move_graphical_pointer(start_x + dx * 8, start_y + dy * 8)
            time.sleep(0.25)
            qmp_command('input-send-event', {'events': [
                {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
            time.sleep(0.4)
            after_release = graphical_task_rect(task_expr)
            expected = (left, right + dx, top, bottom + dy)
            if after_release != expected:
                raise RuntimeError(f'{label} resize did not persist on release: '
                                   f'expected {expected}, got {after_release}')
            time.sleep(0.6)
            after_settle = graphical_task_rect(task_expr)
            if after_settle != expected:
                raise RuntimeError(f'{label} geometry reverted after release: '
                                   f'expected {expected}, got {after_settle}')
            # The original serial shell command itself must not recreate or
            # resize the graphical task.
            command('zstatus', 'desktop:')
            after_command = graphical_task_rect(task_expr)
            if after_command != expected:
                raise RuntimeError(f'{label} geometry changed after another command: '
                                   f'expected {expected}, got {after_command}')
            print(f'PASS {label} corner resize persists: {original} -> {expected}',
                  flush=True)
        finally:
            command(f'zc WinHorz({left},{right},{task_expr}); '
                    f'WinVert({top},{bottom},{task_expr});', 'zc: executed')
    def probe_graphical_terminal_resize():
        probe_graphical_task_resize('bootstrap_gui_terminal_task',
                                    'bootstrap graphical terminal')
        second = command('zc Print("gterm_second_valid=%d\\n", '
                         'TaskValidate(bootstrap_gui_terminal_home_second));',
                         'gterm_second_valid=')
        match = re.search(r'gterm_second_valid=(\d+)', second)
        if not match:
            raise RuntimeError('could not inspect upstream second terminal:\n' + second)
        if int(match.group(1)):
            probe_graphical_task_resize('bootstrap_gui_terminal_home_second',
                                        'upstream User terminal')
        else:
            print('SKIP upstream User terminal resize: normal startup did not create it',
                  flush=True)
    def wait_for_graphical_terminal_banner(timeout=15):
        deadline = time.monotonic() + timeout
        while True:
            screen = graphical_screen('terminal-banner-after-reboot')
            if is_graphical_splash_visible(screen):
                qmp_send_key('esc')
            elif 'ZealOS ARM64 graphical terminal' in screen[3]:
                print('PASS graphical terminal banner rendered after reboot', flush=True)
                return
            if time.monotonic() >= deadline:
                raise RuntimeError('graphical terminal banner did not render after reboot; '
                                   f'see {graphical_path("terminal-banner-after-reboot")}\n'
                                   + screen[3])
            time.sleep(0.1)
    def qmp_send_text(text, delay=0.5, dismiss_splash=True):
        if dismiss_splash:
            dismiss_home_splash_if_visible()
        qcodes = {
            ' ': ('spc',), '/': ('slash',), '.': ('dot',),
            ';': ('semicolon',), ':': ('shift', 'semicolon'), '\n': ('ret',),
            ',': ('comma',),
            '"': ('shift', 'apostrophe'), "'": ('apostrophe',),
            '_': ('shift', 'minus'),
            '%': ('shift', '5'), '+': ('shift', 'equal'),
            '=': ('equal',),
            '(': ('shift', '9'), ')': ('shift', '0'),
            '{': ('shift', 'bracket_left'), '}': ('shift', 'bracket_right'),
            '[': ('bracket_left',), ']': ('bracket_right',),
            '\\': ('backslash',),
            '*': ('shift', '8'), '-': ('minus',),
            '!': ('shift', '1'), '@': ('shift', '2'), '#': ('shift', '3'),
            '$': ('shift', '4'), '^': ('shift', '6'), '&': ('shift', '7'),
            '|': ('shift', 'backslash'), '<': ('shift', 'comma'),
            '>': ('shift', 'dot'), '?': ('shift', 'slash'),
            '`': ('grave_accent',), '~': ('shift', 'grave_accent'),
        }
        for char in text:
            started = time.monotonic()
            keys = qcodes.get(char)
            if keys is None:
                keys = ('shift', char.lower()) if char.isupper() else (char,)
            qmp_send_keys(keys)
            # Keep longer lines below the guest's cooperative input polling
            # rate; QMP acknowledges injection before the source task consumes
            # each event, and an early Return can otherwise overtake the tail.
            time.sleep(max(0, delay - (time.monotonic() - started)))
    def exercise_live_lattice_controls():
        # This runs while the graphical demo owns the guest event loop, so use
        # QMP directly; qmp_send_text also probes the serial shell for splash
        # state and would block until Lattice exits.
        head_box = (150, 190, 171, 211)
        move_graphical_pointer(160, 200)
        graphical_mouse_button('left', True)
        time.sleep(0.1)
        graphical_mouse_button('left', False)
        place_deadline = time.monotonic() + 5
        while True:
            placed = graphical_screen('lattice-live-place')
            placed_head = rgb_coordinates_in(placed, (255, 85, 85), head_box)
            if len(placed_head) >= 5:
                break
            if time.monotonic() >= place_deadline:
                raise RuntimeError('Lattice left-click did not place its red head:\n' +
                                   placed[3])
            time.sleep(0.1)
        move_graphical_pointer(160, 200)
        graphical_mouse_button('right', True)
        time.sleep(0.1)
        move_graphical_pointer(160, 240)
        time.sleep(0.1)
        graphical_mouse_button('right', False)
        aim_deadline = time.monotonic() + 5
        while True:
            aimed = graphical_screen('lattice-live-aim')
            aimed_head = rgb_coordinates_in(aimed, (255, 85, 85), head_box)
            if len(aimed_head) >= 5 and aimed_head != placed_head:
                break
            if time.monotonic() >= aim_deadline:
                raise RuntimeError('Lattice right-drag did not rotate its red head')
            time.sleep(0.1)
        print('PASS live Lattice left-click placement and right-drag aim', flush=True)

        # The bring-up picker cycles a deterministic palette pair per `c`;
        # verify the visible Mid/Edge swatches after the second pair.
        qmp_send_key('c')
        qmp_send_key('c')
        color_deadline = time.monotonic() + 5
        while True:
            colors = graphical_screen('lattice-live-colors')
            mid_red = rgb_pixels_in(colors, (170, 0, 0),
                                    (10, colors[1]-26, 62, colors[1]-10))
            edge_white = rgb_pixels_in(colors, (255, 255, 255),
                                       (74, colors[1]-26, 126, colors[1]-10))
            if mid_red >= 100 and edge_white >= 100:
                break
            if time.monotonic() >= color_deadline:
                raise RuntimeError('Lattice color key did not update Mid/Edge swatches')
            time.sleep(0.1)
        print('PASS live Lattice color key updates both visible swatches', flush=True)

        # Enter clears old strokes and restores the centered default turtle.
        qmp_send_key('ret')
        restart_deadline = time.monotonic() + 5
        while True:
            restarted = graphical_screen('lattice-live-restart')
            center_head = rgb_coordinates_in(
                restarted, (255, 85, 85),
                (restarted[0]//2-10, restarted[1]//2-10,
                 restarted[0]//2+11, restarted[1]//2+11))
            reset_mid = rgb_pixels_in(restarted, (255, 255, 85),
                                      (10, restarted[1]-26,
                                       62, restarted[1]-10))
            if len(center_head) >= 5 and reset_mid >= 100:
                break
            if time.monotonic() >= restart_deadline:
                raise RuntimeError('Lattice Enter did not restore its initial pose/colors')
            time.sleep(0.1)

        # Width zero plots the black edge over its centerline. Widen to two
        # pixels and raise speed once so Space produces visible color pixels.
        qmp_send_keys(['shift', 'equal'])
        qmp_send_keys(['shift', 'equal'])
        qmp_send_key('up')
        qmp_send_key('spc')
        step_deadline = time.monotonic() + 5
        while True:
            stepped = graphical_screen('lattice-live-space-step')
            step_yellow = rgb_pixels_in(
                stepped, (255, 255, 85),
                (stepped[0]//2-2, stepped[1]//2-4,
                 stepped[0]//2+8, stepped[1]//2+5))
            if step_yellow:
                break
            if time.monotonic() >= step_deadline:
                raise RuntimeError('Lattice Space did not leave a visible step mark')
            time.sleep(0.1)
        print('PASS live Lattice restart and visible Space step', flush=True)

    def checked_put_key():
        command('zload /Tests/TaskPutKey.ZC', 'zc: loaded /Tests/TaskPutKey.ZC')
        result = command('zcall TaskPutKeyChecks', 'zc => 0x000000000000002a')
        if '<>' not in result or '~' in result:
            raise RuntimeError('PutKey echo/silent output mismatch: '+result)
        print('PASS PutKey text echo and silent flag', flush=True)
    def checked_task_print():
        command('zload /Tests/TaskPrint.ZC', 'zc: loaded /Tests/TaskPrint.ZC')
        result = command('zcall TaskPrintChecks', 'zc => 0x000000000000002a')
        if '$PT$A$FG$ text % BC\n tail\n' not in result.replace('\r\n', '\n'):
            raise RuntimeError('Print format or varargs output mismatch: '+result)
        print('PASS bare Print format and varargs', flush=True)
    def checked_task_recycling():
        def usage(output):
            match = re.search(r'data=(\d+)/\d+ code=(\d+)/\d+', output)
            if not match: raise RuntimeError('zstatus omitted arena usage: '+output)
            return tuple(map(int, match.groups()))
        before = usage(command('zstatus', 'failed=0'))
        command('zcall TaskTextRecycleChecks', 'zc => 0x000000000000002a')
        after = usage(command('zstatus', 'failed=0'))
        if any(end > start + 4096 for start, end in zip(before, after)):
            raise RuntimeError(f'TaskText recycled filters leaked compiler arena: {before} -> {after}')
        print(f'PASS TaskText compiler arena reclaimed: {before} -> {after}', flush=True)
        for _ in range(16):
            command('zcall TaskCompilerScopeChecks', 'zc => 0x000000000000002a')
        compiled = usage(command('zstatus', 'failed=0'))
        if any(end > start + 4096 for start, end in zip(before, compiled)):
            raise RuntimeError(f'TaskText compiling children leaked compiler arena: {before} -> {compiled}')
        print(f'PASS TaskText compiling children reclaimed: {before} -> {compiled}', flush=True)
    def checked_shell_answers():
        command('zcall ShellAnswerReset', 'zc => 0x000000000000002a')
        command('zc 5; Fs->answer+37;', 'zc: executed')
        command('zcall ShellIntegerAnswerChecks', 'zc => 0x000000000000002a')
        command('zcall ShellAnswerReset', 'zc => 0x000000000000002a')
        command('zc 1.5;', 'zc: executed')
        command('zcall ShellFloatAnswerChecks', 'zc => 0x000000000000002a')
    def checked_background_tasks():
        command('zcall TaskTimerBackgroundStart', 'zc => 0x000000000000002a')
        time.sleep(.08)
        command('zcall TaskTimerBackgroundChecks', 'zc => 0x000000000000002a')
        command('zcall TaskBackgroundKeyStart', 'zc => 0x000000000000002a')
        time.sleep(.03)
        command('zstatus', 'failed=0')
        command('zcall TaskBackgroundKeyCleanup', 'zc => 0x000000000000002a')
        command('zcall TaskBackgroundFocusStart', 'zc => 0x000000000000002a')
        qmp_send_key('g')
        # Give the idle scheduler time to deliver the focused key and end
        # the child before the next shell command under a slow HVF run.
        time.sleep(.2)
        command('zcall TaskBackgroundFocusChecks', 'zc => 0x000000000000002a')
    def command_with_key(line, key, expected):
        # The shell consumes Return before entering zcall; the next serial
        # byte must be read by the native KeyGet binding, not by the shell.
        guest.stdin.write((line+'\r').encode()+key); guest.stdin.flush()
        result = until(b'\n> ')
        if expected not in result: raise RuntimeError(f'{line}: missing {expected!r}\n{result}')
        print(f'PASS {line} + key {key!r}: {expected}', flush=True)
        return result
    def command_after_marker_key(line, marker, key, expected, delay=0,
                                 terminator=b'\r', virtio=False):
        guest.stdin.write(line.encode()+terminator); guest.stdin.flush()
        before = until(marker.encode())
        if delay:
            time.sleep(delay)
        if virtio:
            qmp_send_key(key.decode('ascii'))
        else:
            guest.stdin.write(key); guest.stdin.flush()
        result = before + until(b'\n> ')
        if expected not in result: raise RuntimeError(f'{line}: missing {expected!r}\n{result}')
        print(f'PASS {line} after {marker!r} + key {key!r}: {expected}', flush=True)
        return result
    def editor_command(line, expected, finish=False):
        guest.stdin.write((line+'\r').encode()); guest.stdin.flush()
        result = until(b'\n> ' if finish else b'\nedit> ')
        if expected not in result: raise RuntimeError(f'editor {line}: missing {expected!r}\n{result}')
        print(f'PASS editor {line}: {expected}', flush=True)
        return result
    def upload_source(name, data, checksum=None):
        if checksum is None:
            checksum = zlib.crc32(data)
        guest.stdin.write(f'zrecv {name} {len(data)} {checksum:08x}\r'.encode())
        guest.stdin.flush()
        ready = until(b' chunk=128\r\n')
        if f'zrecv: ready bytes={len(data)}' not in ready:
            raise RuntimeError('zrecv did not become ready:\n'+ready)
        for end in range(128, len(data)+128, 128):
            chunk = data[end-128:end]
            guest.stdin.write(chunk.hex().encode()+b'\r')
            guest.stdin.flush()
            ack = until(f'zrecv: ack {min(end, len(data))}\r\n'.encode())
            if 'zrecv: ack ' not in ack:
                raise RuntimeError('zrecv chunk failed:\n'+ack)
        return until(b'\n> ')
    try:
        # Full cold source compilation includes task-owned compiler metadata.
        # Keep the longer startup allowance separate from command timeouts.
        boot_start = time.monotonic()
        boot = until(b'\n> ', timeout=180)
        for marker in ('hc IR OK (front+host)', 'virtio-blk: rw OK',
                       'zc: native compiler ready',
                       'zc: loaded /Kernel/KMathB.ZC',
                       'zc: upstream ZealOS graphics sources loaded',
                       'zc: boot graphics ODE hooks ready',
                       'zc: source Window Manager started at boot',
                       'zc: graphical terminal started at boot; serial shell remains available'):
            if marker not in boot: raise RuntimeError(f'boot missing {marker}\n{boot[-4000:]}')
        if re.search(r'hc IR FAIL|(?:Upstream|Front) [^\r\n]* FAIL', boot):
            raise RuntimeError('legacy regressions')
        print(f'PASS boot and legacy regression markers ({time.monotonic()-boot_start:.1f}s)', flush=True)
        # Give the cooperative terminal fiber and source manager at least a
        # short window to produce a real visible frame before measuring scanout.
        time.sleep(0.5)
        desktop = command('zstatus', 'desktop: winmgr=1 gterm=1 windowdemo=0 presented_frames=')
        if 'keyboard_focus=' not in desktop:
            raise RuntimeError('zstatus omitted the active guest keyboard-focus task:\n' + desktop)
        if 'keyboard_focus=Terminal' not in desktop:
            raise RuntimeError('boot graphical terminal does not own the VM keyboard:\n' + desktop)
        if 'gui_keyboard: key_events=' not in desktop:
            raise RuntimeError('zstatus omitted graphical keyboard diagnostics:\n' + desktop)
        keyboard_diagnostic = 'usb_input:' if args.usb_input else 'virtio_keyboard: ready=1'
        if keyboard_diagnostic not in desktop:
            raise RuntimeError('zstatus omitted low-level keyboard diagnostics:\n' + desktop)
        if args.usb_input:
            count = (3 if args.usb_pointer == 'tablet' else 2) + args.usb_hubs
            if not re.search(rf'usb_input: controllers=1 devices={count} keyboard=usb pointer=usb-{args.usb_pointer} .* errors=0', desktop):
                raise RuntimeError('native xHCI/HID did not enumerate all three USB inputs:\n'+desktop)
            if not args.usb_with_virtio and 'virtio_keyboard: ready=0' not in desktop:
                raise RuntimeError('USB-only acceptance unexpectedly has a VirtIO keyboard:\n'+desktop)
            print('PASS native USB controller, keyboard, mouse and tablet enumeration', flush=True)
            if args.usb_hubs:
                if not re.search(rf'usb_topology: hubs={args.usb_hubs} .* rejected=0 quarantined=0', desktop):
                    raise RuntimeError('USB hub tree was not enumerated:\n'+desktop)
                route = sum(1 << (4 * tier) for tier in range(args.usb_hubs))
                if not re.search(rf'usb_device: root=\d+ route={route} depth={args.usb_hubs} .* hub_ports=0', desktop):
                    raise RuntimeError('USB keyboard route/depth does not match hub topology:\n'+desktop)
                print(f'PASS native USB enumeration through {args.usb_hubs} nested hubs', flush=True)
            command('usbcheck', 'usbcheck: command/event ring wrap OK')
        frames = re.search(r'desktop: winmgr=1 gterm=1 windowdemo=0 presented_frames=(\d+)', desktop)
        if not frames or int(frames.group(1)) == 0:
            raise RuntimeError('boot graphical terminal has not presented a frame:\n' + desktop)
        if 'gterm_window=0x0000003f task/show/display_doc/put_doc/border_doc/geometry' not in desktop:
            raise RuntimeError('boot graphical terminal window is not fully initialized and visible:\n' + desktop)
        scanout = re.search(r'scanout: nonblack_pixels=(\d+) of 480000', desktop)
        if not scanout:
            raise RuntimeError('zstatus did not report visible framebuffer pixels:\n' + desktop)
        if int(scanout.group(1)) < 1000:
            raise RuntimeError('boot graphical terminal is not painting visible pixels:\n' + desktop)
        print('PASS boot graphical terminal window and framebuffer diagnostics', flush=True)
        if args.probe_doc_includes:
            focus_graphical_terminal()
            run_graphical_terminal_command('doc-includes-ready', '40+2;', 'ans=0x2A=42')
            command('zload /Tests/ProgressAliases.ZC', 'zc: loaded /Tests/ProgressAliases.ZC')
            command('zcall ProgressAliasChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/ListFormat.ZC', 'zc: loaded /Tests/ListFormat.ZC')
            command('zcall ListFormatChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/DocIncludes.ZC', 'zc: loaded /Tests/DocIncludes.ZC')
            for check in ('WriteChecks', 'Checks', 'RecoveryChecks', 'MalformedChecks'):
                command('zcall DocInclude'+check, 'zc => 0x000000000000002a')
            command('zc U8 doc_include_truncated[17]; MemSet(doc_include_truncated,0,17); doc_include_truncated[9]=100; FileWrite("/Tests/DocIncludes/Truncated.ZC",doc_include_truncated,17);', 'zc: executed')
            command('zc I64 DocIncludeBadResource(){return ExeFile("/Tests/DocIncludes/Truncated.ZC");}', 'zc: executed')
            command('zcall DocIncludeBadResource', 'zc => 0x0000000000000000')
            before = command('zstatus', 'failed=0')
            for _ in range(3):
                command('zcall DocIncludeRecoveryChecks', 'zc => 0x000000000000002a')
                command('zcall DocIncludeBadResource', 'zc => 0x0000000000000000')
            after = command('zstatus', 'failed=0')
            heap = r'data=([0-9/]+) code=([0-9/]+) allocations=([0-9]+)'
            if re.search(heap, before).groups() != re.search(heap, after).groups():
                raise RuntimeError('failed nested DolDoc includes leaked managed memory')
            command('zload /Tests/Registry.ZC', 'zc: loaded /Tests/Registry.ZC')
            for check in ('RegistryDocChecks', 'RegistryRecoveryChecks'):
                command('zcall '+check, 'zc => 0x000000000000002a')
            command('zload /Tests/SourceMacros.ZC', 'zc: loaded /Tests/SourceMacros.ZC')
            command('zcall StockSourceMacroChecks', 'zc => 0x000000000000002a')
            command('zstatus', 'failed=0')
            print('PASS nested DolDoc source/binary includes, EOF lifetime, paths and recovery without leaks', flush=True)
            raise SystemExit(0)
        if args.probe_3d_apps:
            command('zload /Tests/ProgramCatalog.ZC',
                    'zc: loaded /Tests/ProgramCatalog.ZC')
            mesh_output = command(
                'zc Print("3d_mesh_fixture_compile=%d\\n",'
                'StockCatalogCompile("/Tests/Sprite3DMesh.ZC"));',
                '3d_mesh_fixture_compile=', timeout=45)
            mesh_compiled = '3d_mesh_fixture_compile=1' in mesh_output
            print(('COMPILES ' if mesh_compiled else 'BLOCKED ')+
                  '/Tests/Sprite3DMesh.ZC', flush=True)
            if mesh_compiled:
                command('zload /Tests/Sprite3DMesh.ZC',
                        'zc: loaded /Tests/Sprite3DMesh.ZC')
                command('zcall Sprite3DMeshChecks',
                        'zc => 0x000000000000002a')
            interpolation_output = command(
                'zc Print("3d_interpolation_fixture_compile=%d\\n",'
                'StockCatalogCompile("/Tests/Sprite3DInterpolation.ZC"));',
                '3d_interpolation_fixture_compile=', timeout=45)
            interpolation_compiled = '3d_interpolation_fixture_compile=1' in interpolation_output
            print(('COMPILES ' if interpolation_compiled else 'BLOCKED ')+
                  '/Tests/Sprite3DInterpolation.ZC', flush=True)
            if interpolation_compiled:
                command('zload /Tests/Sprite3DInterpolation.ZC',
                        'zc: loaded /Tests/Sprite3DInterpolation.ZC')
                command('zcall Sprite3DInterpolationChecks',
                        'zc => 0x000000000000002a')
            app_probes = (
                ('/Apps/GrModels/Run.ZC', '/Apps/GrModels/Run.ZC'),
                ('/Apps/KeepAway/Run.ZC', '/Tests/KeepAwayCompile.ZC'),
                ('/Apps/Titanium/Run.ZC', '/Tests/TitaniumCompile.ZC'),
            )
            app_results = []
            for path, compile_path in app_probes:
                output = command('zc Print("3d_catalog=%d\\n",'
                                 f'StockCatalogCompile("{compile_path}"));',
                                 '3d_catalog=', timeout=45)
                compiled = '3d_catalog=1' in output
                app_results.append({'path': path, 'compiled': compiled,
                                    'compile_path': compile_path,
                                    'output': output})
                print(('COMPILES ' if compiled else 'BLOCKED ')+path,
                      flush=True)
            service_output = command(
                'zc Print("3d_services_compile=%d\\n",'
                'StockCatalogCompile("/Tests/Sprite3DServices.ZC"));',
                '3d_services_compile=', timeout=45)
            services_compiled = '3d_services_compile=1' in service_output
            print(('COMPILES ' if services_compiled else 'BLOCKED ')+
                  '/Tests/Sprite3DServices.ZC', flush=True)
            service_runtime_output = ''
            service_runtime_passed = False
            if services_compiled:
                command('zload /Tests/Sprite3DServices.ZC',
                        'zc: loaded /Tests/Sprite3DServices.ZC')
                service_runtime_output = command(
                    'zcall Sprite3DServiceChecks',
                    'zc =>')
                service_runtime_passed = (
                    'zc => 0x000000000000002a' in service_runtime_output)
            height_output = command(
                'zc Print("3d_height_fixture_compile=%d\\n",'
                'StockCatalogCompile("/Tests/FormatHeight.ZC"));',
                '3d_height_fixture_compile=')
            if '3d_height_fixture_compile=1' not in height_output:
                raise RuntimeError('3D app formatter fixture did not compile:\n'+
                                   height_output)
            command('zload /Tests/FormatHeight.ZC',
                    'zc: loaded /Tests/FormatHeight.ZC')
            command('zcall FormatHeightChecks',
                    'zc => 0x000000000000002a')
            report = {
                'commit': json.loads((ROOT/'upstream/pinned/UPSTREAM.json').read_text())['commit'],
                'mode': 'isolated CCF_JUST_LOAD compile plus runtime service fixture',
                'applications': app_results,
                'mesh_fixture_compiled': mesh_compiled,
                'mesh_fixture_output': mesh_output,
                'interpolation_fixture_compiled': interpolation_compiled,
                'interpolation_fixture_output': interpolation_output,
                'service_fixture_compiled': services_compiled,
                'service_output': service_output,
                'service_runtime_output': service_runtime_output,
                'service_runtime_passed': service_runtime_passed,
            }
            (ROOT/'build/3d-apps-compile.json').write_text(
                json.dumps(report, indent=2)+'\n')
            if (not mesh_compiled or not interpolation_compiled or
                    not services_compiled or not service_runtime_passed or
                    not all(item['compiled'] for item in app_results)):
                print('BLOCKED 3D service/app gate: sprite compile or runtime checks failed',
                      flush=True)
                raise SystemExit(2)
            command('zstatus', 'failed=0')
            focus_graphical_terminal()
            type_graphical_prompt_line(
                '3d-grmodels-launch',
                'ExeFile2("/Apps/GrModels/Run.ZC");')
            qmp_send_key('ret')
            time.sleep(1)
            grmodels = graphical_screen('3d-grmodels-entry')
            print('GRMODELS ENTRY SCREEN\n'+grmodels[3], flush=True)
            move_graphical_pointer(53, 52)
            graphical_mouse_button('left', True)
            time.sleep(0.2)
            graphical_mouse_button('left', False)
            time.sleep(1)
            man_form = graphical_screen('3d-grmodels-man-form')
            print('GRMODELS MAN FORM\n'+man_form[3], flush=True)
            qmp_send_key('esc')
            require_graphical_text(
                '3d-grmodels-man-color-shirt', 'Single-Sided means', timeout=12)

            def click_3d_color(name, current_marker, next_marker):
                # Select the red entry in the first (single-sided, full-range)
                # column. In the 800x600 UI it is the fifth eight-pixel row
                # below the section header.
                deadline = time.monotonic() + 12
                attempt = 0
                while True:
                    screen = graphical_screen(name+'-before')
                    if next_marker in screen[3]:
                        return screen
                    if current_marker not in screen[3]:
                        raise RuntimeError(
                            f'{name}: color picker changed without showing '
                            f'{next_marker!r}:\n'+screen[3])
                    if time.monotonic() >= deadline:
                        diagnostic = command('zstatus', 'failed=')
                        raise RuntimeError(
                            f'{name}: color choice did not advance from '
                            f'{current_marker!r} to {next_marker!r}:\n'+
                            screen[3]+'\n'+diagnostic)
                    attempt += 1
                    time.sleep(0.25)
                    if attempt == 1:
                        move_graphical_pointer(100, 124)
                        graphical_mouse_button('left', True)
                        time.sleep(0.15)
                        graphical_mouse_button('left', False)
                    elif attempt == 2:
                        # PopUpMenu also supports Return on the highlighted
                        # DolDoc link; use it once if the click only hovered.
                        qmp_send_key('ret')
                    else:
                        move_graphical_pointer(100, 124)
                        graphical_mouse_button('left', True)
                        time.sleep(0.15)
                        graphical_mouse_button('left', False)
                    print(f'3D color selection {name} attempt {attempt}', flush=True)
                    time.sleep(0.35)

            click_3d_color('3d-grmodels-man-pants', 'Shirt', 'Pants')
            click_3d_color('3d-grmodels-man-skin', 'Pants', 'Skin')
            click_3d_color('3d-grmodels-man-shoes', 'Skin', 'Shoes')
            click_3d_color('3d-grmodels-man-generated', 'Shoes', 'Do another')
            man_generated = require_graphical_text(
                '3d-grmodels-man-generated', 'Do another', timeout=20)
            if 'Do another' not in man_generated[3]:
                raise RuntimeError('GrModels Man did not finish mesh generation')
            print('PASS GrModels Man mesh generation reached its repeat prompt',
                  flush=True)
            qmp_send_key('n')
            time.sleep(0.5)
            graphical_screen('3d-grmodels-man-after-exit')
            run_graphical_terminal_command(
                '3d-grmodels-man-exit-smoke', '40+2;', 'ans=0x2A=42', timeout=20)
            command('zstatus', 'failed=0')
            mesh_diag = ''.join(command(
                line, 'zc: executed', timeout=20)
                for line in (
                    'zc Print("3d_live_mesh=%d,%d,%d,%d\\n",'
                    'sprite_debug_mesh_last_vertices,'
                    'sprite_debug_mesh_last_triangles,'
                    'sprite_debug_mesh_last_result,'
                    'sprite_debug_mesh_last_work);',
                    'zc Print("3d_live_color_pixels=%d,%d,%d,%d,%d,%d\\n",'
                    'sprite_debug_mesh_last_color,'
                    'sprite_debug_mesh_last_lit_color,'
                    'sprite_debug_mesh_pixel_attempts,'
                    'sprite_debug_mesh_pixel_writes,'
                    'sprite_debug_mesh_pixel_min_x,'
                    'sprite_debug_mesh_pixel_min_y);',
                    'zc Print("3d_live_color_masks=%d,%d,%d\\n",'
                    'sprite_debug_mesh_raw_color_mask,'
                    'sprite_debug_mesh_lit_color_mask,'
                    'sprite_debug_mesh_pixel_color_mask);',
                    'zc Print("3d_live_bounds_masks=%d,%d,%d,%d,%d\\n",'
                    'sprite_debug_mesh_pixel_max_x,'
                    'sprite_debug_mesh_pixel_max_y,'
                    'sprite_debug_type_mask_failures,'
                    'sprite_debug_type_mask_last_type,'
                    'sprite_debug_type_mask_last_size);',
                    'zc Print("3d_live_choices=%d,%d,%d,%d,%d\\n",'
                    'sprite_debug_lighting_choice_count,'
                    'sprite_debug_lighting_choices[0],'
                    'sprite_debug_lighting_choices[1],'
                    'sprite_debug_lighting_choices[2],'
                    'sprite_debug_lighting_choices[3]);',
                    'zc Print("3d_live_target=%d,%d,%d,%d,%d,%d,%d\\n",'
                    'sprite_debug_mesh_target_dc_is_bootstrap,'
                    'sprite_debug_mesh_target_body_is_graphics,'
                    'sprite_debug_mesh_target_body_is_foreground,'
                    'sprite_debug_mesh_target_last_x,'
                    'sprite_debug_mesh_target_last_y,'
                    'sprite_debug_mesh_target_last_pixel,'
                    'sprite_debug_mesh_target_last_index);',
                    'zc Print("3d_live_target_window=%d,%d,%d,%d,%d,%d\\n",'
                    'sprite_debug_mesh_target_left,'
                    'sprite_debug_mesh_target_right,'
                    'sprite_debug_mesh_target_top,'
                    'sprite_debug_mesh_target_bottom,'
                    'sprite_debug_mesh_target_scroll_x,'
                    'sprite_debug_mesh_target_scroll_y);'))
            print('GRMODELS MAN RASTER DIAGNOSTIC\n'+mesh_diag, flush=True)

            type_graphical_prompt_line(
                '3d-grmodels-ball-launch',
                'ExeFile2("/Apps/GrModels/Run.ZC");')
            qmp_send_key('ret')
            require_graphical_text('3d-grmodels-ball-entry', 'Make a CSprite model',
                                   timeout=12)
            move_graphical_pointer(53, 84)
            graphical_mouse_button('left', True)
            time.sleep(0.15)
            graphical_mouse_button('left', False)
            ball_form = require_graphical_text('3d-grmodels-ball-form', 'Longitude Faces',
                                                timeout=12)
            if 'Radius' not in ball_form[3] or 'Lattitude Rings' not in ball_form[3]:
                raise RuntimeError('GrModels Ball form is missing expected fields:\n'+
                                   ball_form[3])
            qmp_send_key('esc')
            require_graphical_text('3d-grmodels-ball-color', 'Single-Sided means',
                                   timeout=12)
            click_3d_color('3d-grmodels-ball-generated',
                           'Single-Sided means', 'Vertices:')
            ball_generated = require_graphical_text(
                '3d-grmodels-ball-generated', 'Vertices:', timeout=25)
            require_graphical_text('3d-grmodels-ball-repeat', 'Do another', timeout=10)
            print('PASS GrModels Ball mesh generation; '+
                  next((line.strip() for line in ball_generated[3].splitlines()
                        if 'Vertices:' in line), 'Vertices reported'), flush=True)
            qmp_send_key('n')
            time.sleep(0.5)
            graphical_screen('3d-grmodels-ball-after-exit')
            run_graphical_terminal_command(
                '3d-grmodels-ball-exit-smoke', '40+2;', 'ans=0x2A=42', timeout=20)
            command('zstatus', 'failed=0')
            mesh_diag = ''.join(command(
                line, 'zc: executed', timeout=20)
                for line in (
                    'zc Print("3d_live_mesh=%d,%d,%d,%d\\n",'
                    'sprite_debug_mesh_last_vertices,'
                    'sprite_debug_mesh_last_triangles,'
                    'sprite_debug_mesh_last_result,'
                    'sprite_debug_mesh_last_work);',
                    'zc Print("3d_live_color_pixels=%d,%d,%d,%d,%d,%d\\n",'
                    'sprite_debug_mesh_last_color,'
                    'sprite_debug_mesh_last_lit_color,'
                    'sprite_debug_mesh_pixel_attempts,'
                    'sprite_debug_mesh_pixel_writes,'
                    'sprite_debug_mesh_pixel_min_x,'
                    'sprite_debug_mesh_pixel_min_y);',
                    'zc Print("3d_live_color_masks=%d,%d,%d\\n",'
                    'sprite_debug_mesh_raw_color_mask,'
                    'sprite_debug_mesh_lit_color_mask,'
                    'sprite_debug_mesh_pixel_color_mask);',
                    'zc Print("3d_live_bounds_masks=%d,%d,%d,%d,%d\\n",'
                    'sprite_debug_mesh_pixel_max_x,'
                    'sprite_debug_mesh_pixel_max_y,'
                    'sprite_debug_type_mask_failures,'
                    'sprite_debug_type_mask_last_type,'
                    'sprite_debug_type_mask_last_size);',
                    'zc Print("3d_live_choices=%d,%d,%d,%d,%d\\n",'
                    'sprite_debug_lighting_choice_count,'
                    'sprite_debug_lighting_choices[0],'
                    'sprite_debug_lighting_choices[1],'
                    'sprite_debug_lighting_choices[2],'
                    'sprite_debug_lighting_choices[3]);',
                    'zc Print("3d_live_target=%d,%d,%d,%d,%d,%d,%d\\n",'
                    'sprite_debug_mesh_target_dc_is_bootstrap,'
                    'sprite_debug_mesh_target_body_is_graphics,'
                    'sprite_debug_mesh_target_body_is_foreground,'
                    'sprite_debug_mesh_target_last_x,'
                    'sprite_debug_mesh_target_last_y,'
                    'sprite_debug_mesh_target_last_pixel,'
                    'sprite_debug_mesh_target_last_index);',
                    'zc Print("3d_live_target_window=%d,%d,%d,%d,%d,%d\\n",'
                    'sprite_debug_mesh_target_left,'
                    'sprite_debug_mesh_target_right,'
                    'sprite_debug_mesh_target_top,'
                    'sprite_debug_mesh_target_bottom,'
                    'sprite_debug_mesh_target_scroll_x,'
                    'sprite_debug_mesh_target_scroll_y);'))
            print('GRMODELS BALL RASTER DIAGNOSTIC\n'+mesh_diag, flush=True)

            raise SystemExit(0)
            raise SystemExit(0)
        if args.probe_budget_program:
            command('zload /Tests/KeyboardMenus.ZC', 'zc: loaded /Tests/KeyboardMenus.ZC')
            command('zcall KeyboardMenuChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/ReloadForward.ZC', 'zc: loaded /Tests/ReloadForward.ZC')
            command('zcall ReloadForwardChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/DocNumbers.ZC', 'zc: loaded /Tests/DocNumbers.ZC')
            command('zcall DocNumberChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/CallBoundaries.ZC', 'zc: loaded /Tests/CallBoundaries.ZC')
            command('zcall CallBoundaryChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/FormReflection.ZC', 'zc: loaded /Tests/FormReflection.ZC')
            command('zcall FormReflectionChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/LastClass.ZC', 'zc: loaded /Tests/LastClass.ZC')
            command('zcall LastClassChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/Calendar.ZC', 'zc: loaded /Tests/Calendar.ZC')
            command('zcall CalendarChecks', 'zc => 0x000000000000002a')
            command('zcall CalendarClockChecks', 'zc => 0x000000000000002a')
            command('zcall CalendarScanChecks', 'zc => 0x000000000000002a')
            command('zcall CalendarBufferChecks', 'zc => 0x000000000000002a')
            before = command('zstatus', 'failed=0')
            command('zcall CalendarBufferChecks', 'zc => 0x000000000000002a')
            command('zcall CalendarScanChecks', 'zc => 0x000000000000002a')
            after = command('zstatus', 'failed=0')
            heap = r'data=([0-9/]+) code=([0-9/]+) allocations=([0-9]+)'
            if re.search(heap, before).groups() != re.search(heap, after).groups():
                raise RuntimeError('date/scanner conversion or recovery leaked managed memory')
            before = command('zstatus', 'failed=0')
            for _ in range(3):
                command('zc class BrokenForm { U8 text[16] format "Field" data; };',
                        'compile failed; current statement discarded')
            after = command('zstatus', 'failed=0')
            if re.search(heap, before).groups() != re.search(heap, after).groups():
                raise RuntimeError('malformed class metadata leaked managed memory')
            epoch = command('zcall CalendarUnixNanoseconds', 'zc => 0x')
            epoch_ns = int(re.search(r'zc => 0x([0-9a-fA-F]+)', epoch)[1], 16)
            if abs(epoch_ns/1e9-time.time()) > 5:
                raise RuntimeError('guest wall clock does not match the host UTC clock')
            command('zc Print("clock=%D %T\\n",Now,Now);', 'clock=')
            command('zload /Tests/ProgramCatalog.ZC', 'zc: loaded /Tests/ProgramCatalog.ZC')
            command('zc CTask *budget_check_task=Spawn(&StockCatalogServer);', 'zc: executed')
            command('zc CJob *budget_check_job=TaskExe(budget_check_task,Fs,"ExeFile2(\\"/Tests/BudgetProgram.ZC\\");",0);', 'zc: executed')
            command('zc Print("budget_checks=%d\\n",JobResGet(budget_check_job));', 'budget_checks=42')
            command('zc Kill(budget_check_task);', 'zc: executed')
            command('zload /Tests/BudgetForm.ZC', 'zc: loaded /Tests/BudgetForm.ZC')
            def budget_click_point(expression):
                output = command(f'zc Print("budget_point=%X\\n",{expression});', 'budget_point=')
                point = int(re.search(r'budget_point=([0-9A-Fa-f]+)', output)[1], 16)
                if point == 0xffffffffffffffff:
                    raise RuntimeError('Budget form/entry not found: '+expression)
                move_graphical_pointer(point & 0xffffffff, point >> 32)
                graphical_mouse_button('left', True)
                time.sleep(.2)
                graphical_mouse_button('left', False)
                time.sleep(.2)
                move_graphical_pointer(700, 500)
            def budget_field(index, value):
                budget_click_point(f'BudgetFormPoint({index})')
                # A real form is also a Server popup; do not treat it as
                # the startup splash and dismiss it before typing.
                qmp_send_text(value, delay=.15, dismiss_splash=False)
            def budget_persistence_check():
                command('zc budget_check_task=Spawn(&StockCatalogServer);', 'zc: executed')
                command('zc budget_check_job=TaskExe(budget_check_task,Fs,"ExeFile2(\\"/Tests/BudgetPersist.ZC\\");",0);', 'zc: executed')
                command('zc Print("budget_persist=%d\\n",JobResGet(budget_check_job));', 'budget_persist=42')
                command('zc Kill(budget_check_task);', 'zc: executed')
            focus_graphical_terminal()
            run_graphical_terminal_command('ready', '40+2;', 'ans=0x2A=42')
            original_rect = graphical_task_rect('bootstrap_gui_terminal_task')
            select_os_menu_row('menu', 'Apps', 9)
            deadline = time.monotonic() + 20
            while True:
                ready = command('zc Print("budgetmenu_ready=%d\\n",MenuEntryFind(bootstrap_gui_terminal_task->cur_menu,"File/Abort")!=NULL);', 'budgetmenu_ready=')
                if 'budgetmenu_ready=1' in ready:
                    break
                if time.monotonic() >= deadline:
                    raise RuntimeError('Budget application menu did not appear')
                time.sleep(.3)
            command('zcall BudgetMenuChecks', 'zc => 0x000000000000002a')
            # Merely hovering an original shortcut used to call an unresolved
            # key-name helper from DrawMenu and permanently stop presentation.
            for root, row in [('File', 0), ('File', 1), ('Edit', 0), ('View', 0)]:
                x = graphical_menu_root_x(root) + 4
                move_graphical_pointer(x, 4)
                time.sleep(.2)
                before_hover = command('zstatus', 'failed=0')
                move_graphical_pointer(x, 12+row*8)
                time.sleep(.3)
                after_hover = command('zstatus', 'failed=0')
                frames = r'presented_frames=(\d+)'
                if int(re.search(frames, after_hover)[1]) <= int(re.search(frames, before_hover)[1]):
                    raise RuntimeError('Budget shortcut hover stopped desktop presentation')
            graphical_screen('menu-shortcut-hover')
            move_graphical_pointer(700,500)
            print('PASS Budget File/Edit/View shortcuts, key labels and continued presentation after hover', flush=True)
            select_os_menu_row('new-entry-menu', 'Edit', 0)
            move_graphical_pointer(700,500)
            require_graphical_text('new-form', 'Credit (from) Acct:', timeout=15)
            for index, value in enumerate(('02/29/2024', '     12.50', 'BANK', 'FOOD', 'UTM Budget entry')):
                budget_field(index, value)
            move_graphical_pointer(700, 500)
            require_graphical_text('filled-form', 'UTM Budget entry')
            qmp_send_key('esc')
            require_graphical_text('created-entry', '12.50:UTM Budget entry')
            select_os_menu_row('save-exit-menu', 'File', 1)
            run_graphical_terminal_command('saved-return', '6*7;', 'ans=0x2A=42')
            if graphical_task_rect('bootstrap_gui_terminal_task') != original_rect:
                raise RuntimeError('Budget did not restore terminal geometry')
            budget_persistence_check()
            select_os_menu_row('reopen-menu', 'Apps', 9)
            require_graphical_text('reopened-entry', '12.50:UTM Budget entry', timeout=20)
            budget_click_point('BudgetEntryPoint()')
            qmp_send_key('ret')
            move_graphical_pointer(700, 500)
            require_graphical_text('edit-form', 'Credit (from) Acct:', timeout=15)
            budget_field(1, '     25.00')
            qmp_send_key('esc')
            require_graphical_text('edited-entry', '25.00:UTM Budget entry')
            qmp_send_keys(['shift', 'esc'])
            run_graphical_terminal_command('abort-return', '40+2;', 'ans=0x2A=42')
            budget_persistence_check()
            select_os_menu_row('cancel-menu', 'Apps', 9)
            require_graphical_text('abort-preserved-entry', '12.50:UTM Budget entry', timeout=20)
            qmp_send_text('n', delay=.15, dismiss_splash=False)
            require_graphical_text('cancel-form', 'Credit (from) Acct:')
            budget_field(4, 'CANCELLED')
            qmp_send_keys(['shift', 'esc'])
            require_graphical_text('cancelled-entry', '12.50:UTM Budget entry')
            require_graphical_text_absent('cancelled-entry-absent', 'CANCELLED')
            select_os_menu_row('abort-exit-menu', 'File', 0)
            run_graphical_terminal_command('cancel-return', '40+2;', 'ans=0x2A=42')
            budget_persistence_check()
            command('zvol', 'flush=yes')
            command('zstatus', 'failed=0')
            guest.terminate()
            guest.wait(timeout=10)
            buf.clear()
            guest = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT, bufsize=0)
            reboot = until(b'\n> ', timeout=180)
            if 'zc: graphical terminal started at boot' not in reboot:
                raise RuntimeError('Budget cold disk-only boot did not reach the desktop')
            command('zload /Tests/ProgramCatalog.ZC', 'zc: loaded /Tests/ProgramCatalog.ZC')
            command('zc CTask *budget_check_task; CJob *budget_check_job;', 'zc: executed')
            budget_persistence_check()
            focus_graphical_terminal()
            select_os_menu_row('cold-menu', 'Apps', 9)
            require_graphical_text('cold-entry', '12.50:UTM Budget entry', timeout=20)
            qmp_send_key('esc')
            run_graphical_terminal_command('cold-return', '40+2;', 'ans=0x2A=42')
            command('zstatus', 'failed=0')
            print('PASS calendar/UTC clock/scanner without leaks; Budget form create/edit/cancel, save/abort, clean exit, reopen and cold disk-only persistence', flush=True)
            raise SystemExit(0)
        if args.probe_logic_program:
            command('zload /Tests/ListFormat.ZC', 'zc: loaded /Tests/ListFormat.ZC')
            command('zcall ListFormatChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/ProgressAliases.ZC', 'zc: loaded /Tests/ProgressAliases.ZC')
            command('zcall ProgressAliasChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/ProgramCatalog.ZC', 'zc: loaded /Tests/ProgramCatalog.ZC')
            command('zc CTask *logic_check_task=Spawn(&StockCatalogServer);', 'zc: executed')
            command('zc CJob *logic_check_job=TaskExe(logic_check_task,Fs,"ExeFile2(\\"/Tests/LogicProgram.ZC\\");",0);', 'zc: executed')
            command('zc Print("logic_checks=%d\\n",JobResGet(logic_check_job));', 'logic_checks=42')
            command('zc Kill(logic_check_task);', 'zc: executed')
            focus_graphical_terminal()
            run_graphical_terminal_command('ready', '40+2;', 'ans=0x2A=42')
            original_rect = graphical_task_rect('bootstrap_gui_terminal_task')
            select_os_menu_row('menu', 'Apps', 8)
            require_graphical_text('gates', '0 Gate:', timeout=20)
            qmp_send_text('ANX', delay=.15)
            require_graphical_text('gate-before-edit', '0 Gate: ANX')
            qmp_send_key('backspace')
            require_graphical_text_absent('gate-erased', '0 Gate: ANX')
            qmp_send_text('D\n\n', delay=.15)
            require_graphical_text('table', 'Table size in hex', timeout=10)
            # Edit in the middle, then exercise both Backspace and forward
            # Delete. Assert scanout, not just the final accepted values.
            qmp_send_text('0x20', delay=.15)
            qmp_send_key('left')
            qmp_send_key('backspace')
            qmp_send_text('1', delay=.15)
            qmp_send_key('end')
            qmp_send_key('ret')
            require_graphical_text('input-a', 'Input A:')
            qmp_send_text('0x3', delay=.15)
            require_graphical_text('input-before-edit', 'Input A: 0x3')
            qmp_send_key('backspace')
            require_graphical_text_absent('input-erased', 'Input A: 0x3')
            qmp_send_text('C\n', delay=.15)
            require_graphical_text('input-corrected', 'Input A: 0xC')
            qmp_send_text('0x0A', delay=.15)
            qmp_send_key('left')
            qmp_send_key('left')
            qmp_send_key('delete')
            qmp_send_key('end')
            qmp_send_key('ret')
            require_graphical_text('input-b-corrected', 'Input B: 0xA')
            qmp_send_key('ret')
            require_graphical_text('output-a', 'Output A:')
            before_errors = command('zstatus', 'failed=0')
            qmp_send_text("'\n)\n", delay=.15)
            after_errors = command('zstatus', 'failed=0')
            errors = r'compiler_recovery: errors=(\d+) active=0'
            if int(re.search(errors, after_errors)[1]) < int(re.search(errors, before_errors)[1])+2:
                raise RuntimeError('Logic did not reject both malformed prompt values')
            move_graphical_pointer(550, 350)
            command('zstatus', 'pointer_xy=550/350')
            qmp_send_text('0x8\n\n', delay=.15)
            time.sleep(1)
            circuit = graphical_screen('and-circuit')
            if (rgb_pixels_in(circuit, (0, 0, 0), (40, 20, 180, 90)) < 100 or
                    rgb_pixels_in(circuit, (170, 0, 0), (40, 20, 180, 90)) < 20 or
                    rgb_pixels_in(circuit, (0, 170, 0), (20, 20, 180, 90)) < 20):
                raise RuntimeError('Digital Logic did not render its gate, wires and input labels')
            command('zc Print("logic_drawing=%d\\n",bootstrap_gui_terminal_task->draw_it!=NULL);', 'logic_drawing=1')
            qmp_send_key('esc')
            time.sleep(.5)
            run_graphical_terminal_command('return', '6*7;', 'ans=0x2A=42')
            if graphical_task_rect('bootstrap_gui_terminal_task') != original_rect:
                raise RuntimeError('Digital Logic did not restore terminal geometry')
            command('zc Print("logic_clean=%d\\n",bootstrap_gui_terminal_task->draw_it==NULL);', 'logic_clean=1')
            select_os_menu_row('menu-reopen', 'Apps', 8)
            require_graphical_text('gates-reopen', '0 Gate:', timeout=20)
            qmp_send_text('OR\n\n', delay=.15)
            require_graphical_text('table-reopen', 'Table size in hex', timeout=10)
            qmp_send_text('0x10\n0xC\n0xA\n\n0xE\n\n', delay=.15)
            time.sleep(1)
            reopened = graphical_screen('or-circuit')
            gate_box = (80, 24, 120, 64)
            if len(rgb_coordinates_in(circuit, (0, 0, 0), gate_box) ^
                   rgb_coordinates_in(reopened, (0, 0, 0), gate_box)) < 12:
                raise RuntimeError('reopened Digital Logic did not render a different OR gate')
            qmp_send_key('esc')
            time.sleep(.5)
            run_graphical_terminal_command('reopen-return', '7*6;', 'ans=0x2A=42')
            command('zc Print("logic_reopen_clean=%d\\n",bootstrap_gui_terminal_task->draw_it==NULL);', 'logic_reopen_clean=1')
            # Original Logic aborts through PutExcept for missing setup
            # values. Previously that helper was unresolved and failed the
            # whole runtime. Check each allocation/cleanup boundary.
            for label, setup in (('no-table', 'AND\n\n\n'),
                                 ('no-inputs', 'AND\n\n0x10\n\n'),
                                 ('no-outputs', 'AND\n\n0x10\n0xC\n0xA\n\n\n')):
                select_os_menu_row(label+'-menu', 'Apps', 8)
                require_graphical_text(label+'-gates', '0 Gate:', timeout=20)
                qmp_send_text(setup, delay=.15)
                run_graphical_terminal_command(label+'-return', '40+2;', 'ans=0x2A=42')
                if graphical_task_rect('bootstrap_gui_terminal_task') != original_rect:
                    raise RuntimeError('Logic abort did not restore geometry: '+label)
                command('zc Print("logic_abort_clean=%d\\n",bootstrap_gui_terminal_task->draw_it==NULL);', 'logic_abort_clean=1')
                command('zstatus', 'failed=0')
            select_os_menu_row('cancel-menu', 'Apps', 8)
            require_graphical_text('cancel-gates', '0 Gate:', timeout=20)
            qmp_send_text('AND\n\n', delay=.15)
            require_graphical_text('cancel-table', 'Table size in hex', timeout=10)
            # Upstream StrGet cancels with Shift+Escape; plain Escape runs
            # the current document entry instead of cancelling the prompt.
            qmp_send_keys(['shift', 'esc'])
            run_graphical_terminal_command('cancel-return', '40+2;', 'ans=0x2A=42')
            command('zc Print("logic_cancel_clean=%d\\n",bootstrap_gui_terminal_task->draw_it==NULL);', 'logic_cancel_clean=1')
            command('zstatus', 'failed=0')
            print('PASS original Digital Logic: ten exact gate bitmaps, AND/OR circuits, visible prompt editing, malformed-value recovery, all setup aborts, Shift+Escape, relaunch and clean exit', flush=True)
            raise SystemExit(0)
        if args.probe_program_catalog:
            command('zload /Tests/SourceMacros.ZC', 'zc: loaded /Tests/SourceMacros.ZC')
            command('zcall StockSourceMacroChecks', 'zc => 0x000000000000002a')
            command('zcall StockNoWarnChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/ProgramCatalog.ZC', 'zc: loaded /Tests/ProgramCatalog.ZC')
            command('zc Print("invalid_no_warn=%d\\n",StockCatalogCompile("/Tests/NoWarnInvalid.ZC"));',
                    'invalid_no_warn=0')
            paths = []
            for directory in sorted((ROOT/'upstream/pinned/Apps').iterdir()):
                entry = directory/('Run.ZC' if (directory/'Run.ZC').exists() else 'Load.ZC')
                paths.append('/'+entry.relative_to(ROOT/'upstream/pinned').as_posix())
            paths += ['/System/Gr/GrMath.ZC', '/System/Gr/GrPrimatives.ZC',
                      '/Demo/Graphics/Doodle.ZC', '/Demo/Graphics/Life.ZC',
                      '/Demo/Games/MassSpring.ZC']
            results = []
            for path in paths:
                result = command('zc Print("catalog_compile=%d\\n", '
                                 f'StockCatalogCompile("{path}"));',
                                 'catalog_compile=', timeout=45)
                passed = 'catalog_compile=1' in result
                results.append({'path': path, 'compiled': passed, 'output': result})
                print(('COMPILES ' if passed else 'BLOCKED ')+path, flush=True)
            (ROOT/'build/program-catalog.json').write_text(json.dumps({
                'commit': json.loads((ROOT/'upstream/pinned/UPSTREAM.json').read_text())['commit'],
                'mode': 'CCF_JUST_LOAD in disposable task; no runtime acceptance',
                'programs': results}, indent=2)+'\n')
            command('zstatus', 'failed=0')
            print('PASS isolated program compilation inventory saved', flush=True)
            raise SystemExit(0)
        if args.probe_stock_interactive:
            command('zload /Tests/SourceMacros.ZC', 'zc: loaded /Tests/SourceMacros.ZC')
            command('zcall StockSourceMacroChecks', 'zc => 0x000000000000002a')
            command('zcall StockNoWarnChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/InlineArrays.ZC', 'zc: loaded /Tests/InlineArrays.ZC')
            command('zcall StockInlineArrayChecks', 'zc => 0x000000000000002a')
            command('zc Print("stock_source_kill=%d\\n",ExeFile2("/Tests/SourceKillBody.ZC"));', 'stock_source_kill=42')
            command('zload /Tests/PopupMenus.ZC', 'zc: loaded /Tests/PopupMenus.ZC')
            command('zcall StockPopupEntryChecks', 'zc => 0x0000000000000001')
            command('zload /Tests/StockPrograms.ZC', 'zc: loaded /Tests/StockPrograms.ZC')
            command('zcall StockClockChecks', 'zc => 0x000000000000002a')
            command('zcall StockQueueChecks', 'zc => 0x000000000000002a')
            command('zcall StockCircleIntrinsicChecks', 'zc => 0x000000000000002a')
            command('zcall StockPolarCircleChecks', 'zc => 0x000000000000002a')
            command('zcall StockBrushChecks', 'zc => 0x000000000000002a')
            focus_graphical_terminal()
            command('zc TaskExe(bootstrap_gui_terminal_task,NULL,"StockMouseReceiver;",1<<JOBf_FREE_ON_COMPLETE);', 'zc: executed')
            command('zc Print("mouse_ready=%d\\n",stock_mouse_ready);', 'mouse_ready=1')
            move_graphical_pointer(200, 240)
            graphical_mouse_button('left', True)
            time.sleep(.25)
            graphical_mouse_button('left', False)
            time.sleep(.25)
            command('zc stock_mouse_finish=TRUE;', 'zc: executed')
            command('zcall StockMouseEdgeChecks', 'zc => 0x000000000000002a')
            original_rect = graphical_task_rect('bootstrap_gui_terminal_task')
            select_os_menu_row('stock-massspring-menu', 'Apps', 6)
            require_graphical_text('stock-massspring-help', 'Left-Click to place', timeout=20,
                                   foreground_threshold=120000)
            time.sleep(1)
            require_graphical_text('stock-massspring-okay', 'OKAY', timeout=15,
                                   foreground_threshold=120000)
            def click_popup(label):
                output = command(f'zc Print("popup_point=%X\\n",StockPopupPoint("{label}"));', 'popup_point=')
                point = int(re.search(r'popup_point=([0-9A-Fa-f]+)', output).group(1), 16)
                if point == 0xffffffffffffffff:
                    raise RuntimeError('popup entry not found: '+label)
                move_graphical_pointer(point & 0xffffffff, point >> 32)
                graphical_mouse_button('left', True)
                time.sleep(.2)
                graphical_mouse_button('left', False)
                time.sleep(.4)
            click_popup('OKAY')
            command('zstatus', 'failed=0')
            command('zc Print("stock_max=%d,%d draw=%d\\n", bootstrap_gui_terminal_task->pix_width, bootstrap_gui_terminal_task->pix_height, bootstrap_gui_terminal_task->draw_it!=NULL);', 'stock_max=800,592 draw=1')
            for x, y in ((200, 240), (420, 240)):
                move_graphical_pointer(x, y)
                graphical_mouse_button('left', True)
                time.sleep(.2)
                graphical_mouse_button('left', False)
                time.sleep(.4)
            move_graphical_pointer(700, 500)
            time.sleep(.2)
            command('zstatus', 'failed=0')
            masses = graphical_screen('stock-massspring-masses')
            for x in (200, 420):
                if rgb_pixels_in(masses, (0, 0, 0), (x-16, 224, x+17, 257)) < 15:
                    raise RuntimeError('Mass Spring did not draw both placed circles')
            move_graphical_pointer(200, 240)
            graphical_mouse_button('right', True)
            time.sleep(.2)
            move_graphical_pointer(420, 240)
            time.sleep(.2)
            graphical_mouse_button('right', False)
            move_graphical_pointer(700, 500)
            initial_black = rgb_coordinates_in(masses, (0, 0, 0), (100, 150, 600, 400))
            initial_width = max(x for x, y in initial_black)-min(x for x, y in initial_black)
            deadline = time.monotonic()+20
            while True:
                spring = graphical_screen('stock-massspring-spring')
                black = rgb_coordinates_in(spring, (0, 0, 0), (100, 150, 600, 400))
                red = rgb_pixels_in(spring, (170, 0, 0), (100, 150, 600, 400))
                if black and red > 30:
                    width = max(x for x, y in black)-min(x for x, y in black)
                    if width < initial_width-30:
                        break
                if time.monotonic() >= deadline:
                    raise RuntimeError('Mass Spring did not visibly pull its two masses together')
                time.sleep(.2)
            print('PASS unchanged Mass Spring mouse placement, spring creation, visible circles/line and physics', flush=True)
            qmp_send_key('ret')
            time.sleep(.5)
            reset = graphical_screen('stock-massspring-reset')
            if rgb_pixels_in(reset, (170, 0, 0), (100, 150, 600, 400)) > 10 or rgb_pixels_in(reset, (0, 0, 0), (100, 150, 600, 400)) > 10:
                raise RuntimeError('Mass Spring restart did not remove its masses and spring')
            qmp_send_key('esc')
            time.sleep(.5)
            run_graphical_terminal_command('stock-massspring-return', '40+2;', 'ans=0x2A=42')
            restored_rect = graphical_task_rect('bootstrap_gui_terminal_task')
            if restored_rect != original_rect:
                raise RuntimeError(f'Mass Spring did not restore window geometry: {original_rect} -> {restored_rect}')
            command('zc Print("stock_draw_restored=%d\\n", bootstrap_gui_terminal_task->draw_it==NULL);', 'stock_draw_restored=1')
            select_os_menu_row('stock-doodle-menu', 'Apps', 5)
            time.sleep(.5)
            # Right release opens the original color picker. Pick RED by its
            # real menu entry, then draw an XOR preview and commit on release.
            move_graphical_pointer(300, 220)
            graphical_mouse_button('right', True)
            time.sleep(.2)
            graphical_mouse_button('right', False)
            picker = require_graphical_text('stock-doodle-picker', 'RED', timeout=10,
                                            foreground_threshold=25000)
            click_popup('__RED')
            time.sleep(.4)
            move_graphical_pointer(160, 180)
            graphical_mouse_button('left', True)
            time.sleep(.2)
            move_graphical_pointer(330, 330)
            time.sleep(.3)
            preview = graphical_screen('stock-doodle-preview')
            if rgb_pixels_in(preview, (170, 0, 0), (140, 160, 350, 350)) < 200:
                raise RuntimeError('Doodle did not show its red drag preview')
            graphical_mouse_button('left', False)
            time.sleep(.5)
            drawn = graphical_screen('stock-doodle-line')
            red = rgb_pixels_in(drawn, (170, 0, 0), (140, 160, 350, 350))
            if red < 200:
                raise RuntimeError(f'Doodle did not retain its selected red stroke: {red} pixels')
            time.sleep(.5)
            retained = graphical_screen('stock-doodle-retained')
            if rgb_pixels_in(retained, (170, 0, 0), (140, 160, 350, 350)) < 200:
                raise RuntimeError('Doodle lost its stroke on compositor refresh')
            qmp_send_key('esc')
            run_graphical_terminal_command('stock-doodle-return', '6*7;', 'ans=0x2A=42')
            cleared = graphical_screen('stock-doodle-cleared')
            if rgb_pixels_in(cleared, (170, 0, 0), (140, 160, 350, 350)) > 10:
                raise RuntimeError('Doodle did not clear its foreground on exit')
            print('PASS unchanged Doodle color picker, drag preview, retained stroke, clear and prompt recovery', flush=True)
            select_os_menu_row('stock-life-menu', 'Apps', 7)
            time.sleep(.5)
            move_graphical_pointer(180, 200)
            graphical_mouse_button('left', True)
            for x, y in ((230, 250), (290, 200), (350, 260), (410, 210)):
                move_graphical_pointer(x, y)
                time.sleep(.25)
            graphical_mouse_button('left', False)
            deadline = time.monotonic()+25
            first = graphical_screen('stock-life-drawn')
            while True:
                time.sleep(.5)
                second = graphical_screen('stock-life-evolved')
                green1 = rgb_coordinates_in(first, (0, 170, 0), (100, 100, 600, 400))
                green2 = rgb_coordinates_in(second, (0, 170, 0), (100, 100, 600, 400))
                if len(green1) > 10 and len(green2) > 10 and len(green1 ^ green2) > 10:
                    break
                if time.monotonic() >= deadline:
                    raise RuntimeError('Game of Life did not draw and evolve its green cells')
                first = second
            qmp_send_key('esc')
            time.sleep(.5)
            run_graphical_terminal_command('stock-life-return', '7*6;', 'ans=0x2A=42')
            command('zstatus', 'failed=0')
            print('PASS unchanged Game of Life mouse drawing, background evolution, cleanup and terminal recovery', flush=True)
            raise SystemExit(0)
        if args.probe_stock_programs:
            command('zload /Tests/StockPrograms.ZC', 'zc: loaded /Tests/StockPrograms.ZC')
            command('zcall StockProgramIntrinsicChecks', 'zc => 0x000000000000002a')
            focus_graphical_terminal()
            command('zc DocPrint(bootstrap_gui_terminal_task->put_doc, "STOCK_CLEAR_SENTINEL\\n> ");',
                    'zc: executed')
            require_graphical_text('stock-netofdots-preclear', 'STOCK_CLEAR_SENTINEL')
            if args.usb_input:
                select_os_menu_row('stock-netofdots-menu', 'Apps', 3)
            else:
                type_graphical_prompt_line('stock-netofdots-command',
                                           '#include "/Demo/Graphics/NetOfDots"')
                qmp_send_key('ret')
            # Only the program's own window counts. The other terminal has
            # red welcome links even when a transient fan has disappeared.
            fan_box = (8, 32, 395, 500)
            deadline = time.monotonic() + 15
            while True:
                screen = graphical_screen('stock-netofdots-drawing')
                red = rgb_pixels_in(screen, (170, 0, 0), fan_box)
                if red >= 300:
                    break
                if time.monotonic() >= deadline:
                    raise RuntimeError(f'NetOfDots fan is missing from its window: {red} red pixels')
                time.sleep(.1)
            require_graphical_text_absent('stock-netofdots-cleared', 'STOCK_CLEAR_SENTINEL')
            command('zstatus', 'failed=0')
            # Foreground dots can cover individual glyph pixels, so strict
            # font OCR is unsuitable for the overlapped PRESS A KEY line.
            # Check its actual output and the separately captured fan instead.
            if b'PRESS A KEY' not in log_path.read_bytes():
                raise RuntimeError('NetOfDots did not reach its upstream key-wait prompt')
            time.sleep(.75)
            retained = graphical_screen('stock-netofdots-retained')
            retained_red = rgb_pixels_in(retained, (170, 0, 0), fan_box)
            if red < 300 or retained_red < 300:
                raise RuntimeError(f'unchanged NetOfDots did not draw its red fan: {red} red pixels')
            print(f'PASS unchanged NetOfDots rendered red fan ({red} pixels) and waits for input', flush=True)
            qmp_send_key('spc')
            run_graphical_terminal_command('stock-netofdots-return', '40+2;', 'ans=0x2A=42')
            cleared = graphical_screen('stock-netofdots-exit-cleared')
            if rgb_pixels_in(cleared, (170, 0, 0), fan_box) > 10:
                raise RuntimeError('NetOfDots DCFill did not clear its retained foreground')
            select_os_menu_row('stock-lines-menu', 'Apps', 4)
            deadline = time.monotonic() + 60
            # The original draws its first 1000 lines in BLACK, then advances
            # the palette index. Wait for its real animation rather than
            # changing its cadence or replacing the program with a fixture.
            time.sleep(2)  # Let the menu close before sampling animation.
            first = graphical_screen('stock-lines-frame-1')
            while True:
                time.sleep(.5)
                second = graphical_screen('stock-lines-frame-2')
                if 'ERR:/Demo/Graphics/Lines' in second[3]:
                    raise RuntimeError('unchanged Lines failed:\n' + second[3])
                changed = sum(first[2][(y * 800 + x) * 3 + c] !=
                              second[2][(y * 800 + x) * 3 + c]
                              for y in range(fan_box[1], fan_box[3])
                              for x in range(fan_box[0], fan_box[2]) for c in range(3))
                blue = rgb_pixels_in(second, (0, 0, 170), fan_box)
                if changed >= 100 and blue > 20:
                    break
                if time.monotonic() >= deadline:
                    raise RuntimeError(f'unchanged Lines did not animate: {changed} changed channel bytes')
                first = second
            print(f'PASS unchanged Lines animated ({changed} changed channel bytes)', flush=True)
            qmp_send_key('esc')
            run_graphical_terminal_command('stock-lines-return', '6*7;', 'ans=0x2A=42')
            command('zstatus', 'failed=0')
            print('PASS unchanged stock NetOfDots/Lines include, visible drawing, input exit and prompt recovery', flush=True)
            raise SystemExit(0)
        if args.probe_gui_editor_text:
            editor_path = '/Tests/GuiInputProbe.ZC'
            source = 'I64 GuiInputProbe(){return 40+2;}'
            command('zput '+editor_path.lstrip('/')+' \\n',
                    'zput: saved '+editor_path.lstrip('/'))
            focus_graphical_terminal()
            type_graphical_prompt_line('gui-editor-text-command',
                                       'edit '+editor_path)
            qmp_send_key('ret')
            command('zstatus', 'gui_editor_active=1')
            move_graphical_pointer(200, 200)
            graphical_mouse_button('left', True)
            time.sleep(0.15)
            graphical_mouse_button('left', False)
            qmp_send_text(source)
            require_graphical_text('gui-editor-text-visible', source, timeout=8)
            qmp_send_key('esc')
            command('zstatus', 'gui_editor_active=0')
            command('zload disk:'+editor_path,
                    'zc: loaded disk:'+editor_path)
            command('zcall GuiInputProbe', 'zc => 0x000000000000002a')
            print('PASS GUI document editor accepted visible typed source, saved it, and ran the saved file', flush=True)
            raise SystemExit(0)
        if args.probe_screen_depth:
            command('zcall SpriteScreenDepthChecks',
                    'zc => 0x000000000000002a')
            print('PASS screen-coordinate task depth and overlapping-window reset',
                  flush=True)
            raise SystemExit(0)
        if args.probe_lattice_controls:
            focus_graphical_terminal()
            qmp_send_text('latticeplay')
            qmp_send_key('ret')
            require_graphical_text('lattice-live-hud', 'Layer:', timeout=10,
                                   foreground_threshold=215000,
                                   scanline_offset=0)
            exercise_live_lattice_controls()
            qmp_send_key('esc')
            require_graphical_text('lattice-returns-to-terminal',
                                   'latticeplay ok', timeout=10)
            recovered = command('zstatus', 'failed=0')
            if 'gterm=1' not in recovered:
                raise RuntimeError('Lattice Escape did not restore the graphical terminal:\n' +
                                   recovered)
            print('PASS focused live Lattice controls and Escape recovery', flush=True)
            raise SystemExit(0)
        if args.probe_grblot3:
            command('zload /Tests/GrBlot3.ZC',
                    'zc: loaded /Tests/GrBlot3.ZC')
            heap_pattern = r'data=([0-9]+)/[0-9]+ code=([0-9]+)/[0-9]+ allocations=([0-9]+)'
            before = command('zstatus', 'failed=0')
            before_usage = re.search(heap_pattern, before).groups()
            for _ in range(3):
                command('zcall GrBlot3Checks',
                        'zc => 0x000000000000002a', timeout=30)
            after = command('zstatus', 'failed=0')
            after_usage = re.search(heap_pattern, after).groups()
            if any(int(end) > int(start) + 4096
                   for start, end in zip(before_usage, after_usage)):
                raise RuntimeError('repeated GrBlot3 calls leaked compiler arena: '
                                   f'{before_usage} -> {after_usage}')
            print('PASS boot-loaded GrBlot3 transparent, transformed and depth-buffer paths; '
                  f'no arena growth ({before_usage} -> {after_usage})',
                  flush=True)
            raise SystemExit(0)
        if args.probe_grline_cdc:
            command('zload /Tests/GrLineCdc.ZC',
                    'zc: loaded /Tests/GrLineCdc.ZC')
            command('zcall GrLineCdcChecks',
                    'zc => 0x000000000000002a')
            print('PASS bounded CDC line and brush rasterization', flush=True)
            raise SystemExit(0)
        if args.probe_graphics_palette:
            command('zload /Tests/GrSpriteBridge.ZC',
                    'zc: loaded /Tests/GrSpriteBridge.ZC')
            command('zcall GrSpriteCustomPaletteChecks',
                    'zc => 0x000000000000002a')
            print('PASS indexed plot, rectangle, line and text primitives use the active palette',
                  flush=True)
            raise SystemExit(0)
        if args.probe_sprite_arrow_symmetry:
            command('zload /Tests/GrSpriteArrowSymmetry.ZC',
                    'zc: loaded /Tests/GrSpriteArrowSymmetry.ZC')
            command('zcall GrSpriteArrowSymmetryChecks',
                    'zc => 0x000000000000002a')
            print('PASS bounded sprite arrow shaft and both arrowhead arms reflect across a 2D symmetry axis',
                  flush=True)
            raise SystemExit(0)
        redraws = command('zcall BootstrapGuiTerminalRedrawCount', 'zc => 0x')
        redraw_count = re.search(r'zc => 0x([0-9a-fA-F]+)', redraws)
        if not redraw_count or int(redraw_count.group(1), 16) == 0:
            raise RuntimeError('boot compositor has not redrawn the terminal document:\n' + redraws)
        print(f'PASS boot terminal document redraws: {int(redraw_count.group(1), 16)}', flush=True)
        if args.probe_dvd_live:
            source = command('zc Print("home_boot_source=%d\\n",sys_boot_src);',
                             'home_boot_source=4')
            if 'home_boot_source=4' not in source:
                raise RuntimeError('ISO did not boot through the DVD source path:\n' + source)
            focus_graphical_terminal()
            require_graphical_text('dvd-live-installer-status',
                                   'ARM64 live session is ready.')
            require_graphical_text('dvd-live-installer-status',
                                   'Install to a 128 MiB UTM/QEMU VirtIO disk:')
            qmp_send_text('help')
            startup_typing = require_graphical_text('dvd-live-typing', '> help')
            require_graphical_caret(startup_typing, '> help')
            qmp_send_key('ret')
            require_graphical_text('dvd-live-help', 'Graphical commands:')
            print('PASS ISO/DVD startup reaches the graphical live terminal, reports installer status, and accepts input', flush=True)
            raise SystemExit(0)
        if args.probe_source_editor:
            editor_command('zedit App/EditorScanout.ZC',
                           'zedit: new App/EditorScanout.ZC')
            require_graphical_text('zedit-navigation-controls',
                                   'j/k line  [/] page  g/G top/end',
                                   scanline_offset=0)
            require_graphical_text('zedit-save-controls',
                                   'p [line [count]]  w save  wq save+close',
                                   scanline_offset=0)
            editor_command('a I64 EditorScanout(){return 40+2;}',
                           'zedit: added line 1')
            require_graphical_text('zedit-visible-source',
                                   '1:> I64 EditorScanout(){return 40+2;}',
                                   scanline_offset=0)
            require_graphical_text('zedit-modified-state', '[modified]',
                                   scanline_offset=0)
            editor_command('wq', 'zedit: saved App/EditorScanout.ZC', finish=True)
            command('zload disk:App/EditorScanout.ZC',
                    'zc: loaded disk:App/EditorScanout.ZC')
            command('zcall EditorScanout', 'zc => 0x000000000000002a')
            editor_command('zedit App/EditorScanout.ZC',
                           'zedit: opened App/EditorScanout.ZC')
            require_graphical_text('zedit-reopened-source',
                                   '1:> I64 EditorScanout(){return 40+2;}',
                                   scanline_offset=0)
            require_graphical_text('zedit-saved-state', '[saved]',
                                   scanline_offset=0)
            editor_command('q', 'zedit: closed', finish=True)
            print('PASS full-screen source editor controls, visible edits, save/reopen and execution', flush=True)
            raise SystemExit(0)
        if args.probe_gui_windowdemo_input:
            focus_graphical_terminal()
            qmp_send_text('edit /System/DolDoc/DocEd.ZC')
            qmp_send_key('ret')
            time.sleep(0.5)
            editor_open = command('zstatus', 'gui_editor_active=1')
            if 'keyboard_focus=Terminal' not in editor_open:
                raise RuntimeError('GUI editor lost terminal focus in isolated restore probe:\n' + editor_open)
            for key in ('left', 'right', 'esc'):
                qmp_send_key(key)
            editor_closed = command('zstatus', 'gui_editor_active=0')
            if 'keyboard_focus=Terminal' not in editor_closed:
                raise RuntimeError('GUI editor did not restore terminal focus:\n' + editor_closed)
            for key in ('z', 'l', 's', 'ret'):
                qmp_send_key(key)
            command('zcall BootstrapGuiTerminalLastCommandIsZls',
                    'zc => 0x0000000000000001')
            qmp_send_text('windowdemo')
            qmp_send_key('ret')
            command('zstatus', 'desktop: winmgr=1 gterm=1 windowdemo=1 presented_frames=')
            command('windowdemostop',
                    'windowdemo: stopped; bootstrap shell display resumed')
            restored = command('zstatus',
                               'desktop: winmgr=1 gterm=1 windowdemo=0 presented_frames=')
            if 'keyboard_focus=Terminal' not in restored:
                raise RuntimeError('window demo did not restore terminal focus:\n' + restored)
            for key in ('z', 'l', 's', 'ret'):
                qmp_send_key(key)
            require_graphical_prompt_text('windowdemo-poststop-zls', 'zls')
            command('zcall BootstrapGuiTerminalLastCommandIsZls',
                    'zc => 0x0000000000000001')
            qmp_send_text('help')
            caret_screen = require_graphical_text('windowdemo-terminal-caret-text', '> help')
            require_graphical_caret(caret_screen, '> help')
            qmp_send_key('esc')
            command('zc I64 compiler_recovery_committed=0;', 'zc: executed')
            option_command = 'zc Option(OPTf_DECIMAL_ONLY,ON);'
            qmp_send_text(option_command)
            input_state = command('zcall BootstrapGuiTerminalInputDiag', 'zc => 0x')
            input_match = re.search(r'zc => 0x([0-9a-fA-F]+)', input_state)
            input_diag = int(input_match.group(1), 16) if input_match else -1
            if ((input_diag & 255) != len(option_command) or
                    ((input_diag >> 8) & 255) != 1):
                raise RuntimeError('post-windowdemo GUI command buffer is incomplete:\n' + input_state)
            print(f"INFO post-windowdemo input: length={input_diag & 255} "
                  f"last_edit={(input_diag >> 8) & 255}", flush=True)
            require_graphical_text('windowdemo-terminal-echo', '> ' + option_command)
            qmp_send_key('ret')
            rejected_hex_command = 'zc compiler_recovery_committed=0x2a;'
            qmp_send_text(rejected_hex_command)
            require_graphical_text('windowdemo-terminal-rejected-hex-echo',
                                   '> ' + rejected_hex_command)
            qmp_send_key('ret')
            require_graphical_text('windowdemo-terminal-option-error',
                                   'Hex constant disabled by')
            require_graphical_text('windowdemo-terminal-option-name',
                                   'OPTf_DECIMAL_ONLY.')
            print('PASS long terminal input and compiler diagnostics after windowdemo', flush=True)
            raise SystemExit(0)
        if args.probe_home_autostart:
            command('zload /Tests/HomeAutoStart.ZC', 'zc: loaded /Tests/HomeAutoStart.ZC')
            command('zcall HomeAutoStartChecks', 'zc => 0x000000000000002a')
            command('zcall CtrlAltFontDispatchChecks',
                    'zc => 0x000000000000002a')
            ctrl_alt_map = command('zcall CtrlAltFontMapChecks',
                                   'zc => 0x000000000000002a')
            if 'Ctrl-Alt-F: Cmd /Toggle Aux Font' not in ctrl_alt_map:
                raise RuntimeError('Ctrl-Alt shortcut help omitted the font toggle:\n' + ctrl_alt_map)
            key_map = command('zcall KeyMapFamilyChecks',
                              'zc => 0x000000000000002a')
            for binding in ('Left/Right: edit command line',
                            'Home/End: move to start/end',
                            'Up: recall previous command',
                            'Down: clear command line',
                            'Return: run command'):
                if binding not in key_map:
                    raise RuntimeError(f'bootstrap key map omitted {binding!r}:\n' + key_map)
            require_graphical_splash('home-autostart-splash')
            qmp_send_key('esc')
            require_graphical_splash_absent('home-autostart-splash-dismissed')
            command('zload /Tests/PopupWindow.ZC', 'zc: loaded /Tests/PopupWindow.ZC')
            command('zcall PopupWindowOpen', 'zc => 0x000000000000002a')
            # The caret can cover the first P at the DocTerm insertion point.
            require_graphical_text('home-popup-window', 'OPUP WINDOW READY')
            command('zcall PopupWindowClose', 'zc => 0x000000000000002a')
            command('zload /Tests/SplashWindow.ZC',
                    'zc: loaded /Tests/SplashWindow.ZC')
            command('zcall SplashWindowOpen',
                    'zc => 0x000000000000002a')
            require_graphical_splash('home-splash-window')
            qmp_send_key('esc')
            command('zcall SplashWindowWaitForExit',
                    'zc => 0x000000000000002a')
            focus_graphical_terminal()
            qmp_send_keys(['ctrl', 'alt', 'f'])
            command('zcall CtrlAltFontStateAux',
                    'zc => 0x000000000000002a')
            qmp_send_keys(['ctrl', 'alt', 'f'])
            command('zcall CtrlAltFontStateStandard',
                    'zc => 0x000000000000002a')
            print('PASS live Ctrl-Alt-F event toggles auxiliary and standard fonts', flush=True)
            qmp_send_text('help')
            startup_typing = require_graphical_text('home-autostart-typing', '> help')
            require_graphical_caret(startup_typing, '> help')
            qmp_send_key('ret')
            require_graphical_text('home-autostart-help', 'Graphical commands:')
            print('PASS HomeSys disk startup tiles both terminals, runs Once and source PopUp/ZSplash windows, then restores terminal input', flush=True)
            raise SystemExit(0)
        if args.probe_autocomplete:
            command('zload /Tests/AutoCompleteChecks.ZC',
                    'zc: loaded /Tests/AutoCompleteChecks.ZC')
            command('zcall AutoCompleteBootChecks',
                    'zc => 0x000000000000002a')
            print('PASS boot autocomplete corpus indexing and task startup', flush=True)
            # Exercise the real foreground key handler and AutoComplete task,
            # not just the dictionary API. The fixture word has a unique
            # prefix, so Ctrl+F1 should insert exactly its remaining suffix.
            focus_graphical_terminal()
            prefix = 'ZealAutoFixtureWord7'
            completion = prefix + 'Z'
            qmp_send_text(prefix)
            require_graphical_prompt_text('autocomplete-prefix', prefix)
            window_state = command('zcall AutoCompleteWindowDiag', 'zc => 0x')
            match = re.search(r'zc => 0x([0-9a-fA-F]+)', window_state)
            if not match:
                raise RuntimeError('could not inspect live autocomplete popup window:\n' + window_state)
            packed = int(match.group(1), 16)
            popup = {
                'left': packed & 127,
                'right': (packed >> 7) & 127,
                'top': (packed >> 14) & 127,
                'bottom': (packed >> 21) & 127,
                'z': (packed >> 28) & 63,
                'shown': bool((packed >> 34) & 1),
            }
            doc_state = command('zcall AutoCompleteDocumentDiag', 'zc => 0x')
            doc_match = re.search(r'zc => 0x([0-9a-fA-F]+)', doc_state)
            if not doc_match:
                raise RuntimeError('could not inspect autocomplete popup document:\n' + doc_state)
            doc_packed = int(doc_match.group(1), 16)
            print(f"INFO autocomplete popup document size: "
                  f"{doc_packed & 65535}x{(doc_packed >> 16) & 65535}", flush=True)
            print(f"INFO autocomplete popup geometry: {popup}", flush=True)
            if (popup['right'] - popup['left'] < 8 or
                    popup['bottom'] - popup['top'] < 2 or
                    not popup['shown'] or popup['z'] == 0):
                raise RuntimeError(f'autocomplete popup has invalid visible geometry: {popup}')
            qmp_send_keys(['ctrl', 'f1'])
            require_graphical_prompt_text('autocomplete-fill-in', completion)
            qmp_send_key('esc')
            print('PASS graphical autocomplete suggestion and Ctrl+F1 fill-in', flush=True)
            command('zcall AutoCompleteChecks', 'zc => 0x000000000000002a')
            print('PASS upstream autocomplete corpus indexing and task teardown', flush=True)
            raise SystemExit(0)
        if args.probe_input_edges:
            focus_graphical_terminal()
            text = 'Print("Input edges %d\\n",40+2);'
            qmp_send_text(text, delay=0.1)
            typed = require_graphical_text('input-edges-typing', text)
            require_graphical_caret(typed, text)
            qmp_send_key('ret')
            require_graphical_text('input-edges-answer', 'Input edges 42')
            qmp_key_edges(['shift', '9'], True)
            time.sleep(0.6)
            require_graphical_text('input-edges-held', '> ((' if args.usb_input else '> (')
            qmp_key_edges(['9', 'shift'], False)
            qmp_send_key('esc')
            qmp_send_text('42;')
            typed = require_graphical_text('input-edges-recovered', '> 42;')
            require_graphical_caret(typed, '> 42;')
            qmp_send_key('ret')
            require_graphical_text('input-edges-recovered-answer', 'ans=0x2A=42')
            command('zstatus', 'keyboard_state: shift=0 ctrl=0 usb_modifiers=0 usb_repeat=0')
            print('PASS visible mixed-case/symbol input, held Shift+9, release and prompt recovery', flush=True)
            raise SystemExit(0)
        if args.probe_home_once:
            command('zc Print("home_boot_source=%d\\n",sys_boot_src);',
                    'home_boot_source='+('3' if args.pci else '4'))
            command('zload /Tests/HomeOnce.ZC', 'zc: loaded /Tests/HomeOnce.ZC')
            if args.pci:
                command('zcall HomeOnceWaitForAutocomplete',
                        'zc => 0x000000000000002a')
            # Warm up source compilation and child-task allocator slots before
            # taking the no-growth baseline. The third fresh source execution
            # can legitimately populate reusable compiler arenas.
            for _ in range(3):
                for check in ('IncludeChecks', 'TipChecks'):
                    command('zcall HomeOnce'+check,
                            'zc => 0x000000000000002a')
            heap = r'data=(\d+)/\d+ code=(\d+)/\d+ allocations=(\d+)'
            before = command('zstatus', 'failed=0')
            before_usage = tuple(map(int, re.search(heap, before).groups()))
            for _ in range(3):
                for check in ('IncludeChecks', 'TipChecks'):
                    command('zcall HomeOnce'+check,
                            'zc => 0x000000000000002a')
            after = command('zstatus', 'failed=0')
            after_usage = tuple(map(int, re.search(heap, after).groups()))
            if any(after > before for before, after in zip(before_usage, after_usage)):
                raise RuntimeError('home include/tip/error child teardown grew managed memory: '
                                   f'{before_usage} -> {after_usage}')
            print('PASS warmed Home/Once include, tip and error-recovery checks '
                  f'do not grow managed memory: {before_usage} -> {after_usage}', flush=True)
            command('zcall HomeOnceStartChecks', 'zc => 0x000000000000002a')
            if not args.pci:
                require_graphical_text('home-once-dvd-live-status',
                                       'ARM64 live session is ready.')
            command('zcall HomeOnceFinishChecks', 'zc => 0x000000000000002a')
            if not args.pci:
                require_graphical_text('home-once-dvd-installer-status',
                                       'Install to a 128 MiB UTM/QEMU VirtIO disk:')
            focus_graphical_terminal()
            qmp_send_text('40+2;')
            typed = require_graphical_text('home-once-typing', '> 40+2;')
            require_graphical_caret(typed, '> 40+2;')
            qmp_send_key('ret')
            require_graphical_text('home-once-answer', 'ans=0x2A=42')
            command('zcall HomeOnceCloseChecks', 'zc => 0x000000000000002a')
            if args.pci:
                command('zcall HomeOnceSavedStartChecks', 'zc => 0x000000000000002a')
                command('zcall HomeOnceSavedFinishChecks', 'zc => 0x000000000000002a')
                require_graphical_text('home-once-saved', 'Saved home Once 42')
                command('zcall HomeOnceCloseChecks', 'zc => 0x000000000000002a')
                guest.terminate()
                try:
                    guest.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    guest.kill(); guest.wait()
                guest = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, bufsize=0)
                buf.clear()
                until(b'\n> ', timeout=300 if args.probe_home_once else 180)
                command('zload /Tests/HomeOnce.ZC', 'zc: loaded /Tests/HomeOnce.ZC')
                command('zcall HomeOnceReopenStartChecks', 'zc => 0x000000000000002a')
                command('zcall HomeOnceSavedFinishChecks', 'zc => 0x000000000000002a')
                require_graphical_text('home-once-reboot', 'Saved home Once 42')
                command('zcall HomeOnceCloseChecks', 'zc => 0x000000000000002a')
                print('PASS customized Home/Once.ZC wins over root/bare files and survives reboot', flush=True)
            print('PASS original Once include, real boot-source branch, tips and prompt recovery', flush=True)
            raise SystemExit(0)
        if args.probe_registry:
            command('zc Print("registry_writable=%d\\n",ZcSourceWritable("~/Registry.ZC"));',
                    'registry_writable='+('1' if args.pci else '0'))
            if not args.pci:
                command('zc CDoc *registry_readonly=DocNew; '
                        'StrCopy(registry_readonly->filename.name,"/Home/RegistryReadOnly.ZC"); '
                        'DocPrint(registry_readonly,"40+2;"); '
                        'Print("registry_write=%d\\n",DocWrite(registry_readonly)); '
                        'DocDel(registry_readonly);', 'registry_write=0')
            command('zload /Tests/Registry.ZC', 'zc: loaded /Tests/Registry.ZC')
            for check in ('DocChecks', 'TreeChecks', 'RecoveryChecks', 'Checks', 'OnceChecks', 'KillChecks', 'TaskChecks'):
                name = 'Registry'+check
                command('zcall '+name, 'zc => 0x000000000000002a')
            if args.pci:
                command('zcall RegistryPersistChecks', 'zc => 0x000000000000002a')
            before = command('zstatus', 'failed=0')
            for _ in range(3):
                for check in ('TaskChecks', 'TreeChecks', 'RecoveryChecks', 'KillChecks'):
                    command('zcall Registry'+check, 'zc => 0x000000000000002a')
            after = command('zstatus', 'failed=0')
            heap = r'data=([0-9/]+) code=([0-9/]+) allocations=([0-9]+)'
            if re.search(heap, before).groups() != re.search(heap, after).groups():
                raise RuntimeError('registry document/compiler/task teardown leaked managed memory')
            print('PASS registry execution, error recovery and killed-child cleanup preserve managed memory', flush=True)
            if args.probe_gui_keyboard:
                focus_graphical_terminal()
                text = 'Print("R%d\\n",RegistryTreeChecks);'
                qmp_send_text(text)
                typed = require_graphical_text('registry-typing', text)
                require_graphical_caret(typed, text)
                qmp_send_key('ret')
                require_graphical_text('registry-device-answer', 'R42')
                command('zstatus', 'keyboard_state: shift=0 ctrl=0 usb_modifiers=0 usb_repeat=0')
                require_graphical_pointer()
            if args.pci:
                command('zcall RegistryQueueChecks', 'zc => 0x000000000000002a')
                guest.terminate()
                try:
                    guest.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    guest.kill(); guest.wait()
                guest = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, bufsize=0)
                buf.clear()
                until(b'\n> ', timeout=180)
                command('zload /Tests/Registry.ZC', 'zc: loaded /Tests/Registry.ZC')
                command('zcall RegistryRebootChecks', 'zc => 0x000000000000002a')
                if args.probe_gui_keyboard:
                    desktop = command('zstatus', 'desktop: winmgr=1 gterm=1')
                    if not re.search(r'presented_frames=[1-9][0-9]*', desktop):
                        raise RuntimeError('graphical terminal stopped presenting after reboot:\n'+desktop)
                    require_graphical_text('registry-reboot-terminal',
                                            'ZealOS ARM64 graphical terminal')
                print('PASS saved registry and Once/System + Once/User survive reboot and flush after execution', flush=True)
            raise SystemExit(0)
        if args.probe_user_text:
            command('zload /Tests/UserText.ZC', 'zc: loaded /Tests/UserText.ZC')
            for check in ('StartChecks', 'OrderChecks', 'LongChecks', 'BarrierChecks', 'RecoveryChecks'):
                try:
                    command('zcall UserText'+check, 'zc => 0x000000000000002a', timeout=60)
                except RuntimeError as error:
                    if 'guest synchronous fault' in str(error) or 'timeout waiting' in str(error):
                        raise
                    command('zstatus', 'failed=')
                    command('zc Print("user_text_debug value=%d flags=%X filter=%X queue=%X self=%X\\n",user_text_value,user_text_terminal->task_flags,user_text_terminal->last_input_filter_task,user_text_terminal->server_ctrl.next_waiting,&user_text_terminal->server_ctrl);', 'user_text_debug')
                    graphical_screen('user-text-failure')
                    raise
            dismiss_home_splash_if_visible()
            require_graphical_text('user-text-answer', 'XTalk 42')
            if args.probe_gui_keyboard:
                device_command = 'Print("Device %d\\n",40+2);'
                qmp_send_text(device_command)
                typed = require_graphical_text('user-text-typing', device_command)
                require_graphical_caret(typed, device_command)
                qmp_send_key('ret')
                require_graphical_text('user-text-device-answer', 'Device 42')
            command('zcall UserTextCloseChecks', 'zc => 0x000000000000002a')
            before = command('zstatus', 'failed=0')
            for _ in range(2):
                for check in ('StartChecks', 'OrderChecks', 'LongChecks', 'BarrierChecks', 'RecoveryChecks', 'CloseChecks'):
                    command('zcall UserText'+check, 'zc => 0x000000000000002a', timeout=60)
            after = command('zstatus', 'failed=0')
            heap = r'data=(\d+)/\d+ code=(\d+)/\d+ allocations=(\d+)'
            before_usage = re.search(heap, before)
            after_usage = re.search(heap, after)
            if not before_usage or not after_usage:
                raise RuntimeError('zstatus omitted managed-memory usage during XTalk teardown')
            before_usage = tuple(map(int, before_usage.groups()))
            after_usage = tuple(map(int, after_usage.groups()))
            if any(after_value > before_value
                   for before_value, after_value in zip(before_usage, after_usage)):
                raise RuntimeError('XTalk terminal/filter teardown grew managed memory: '
                                   f'{before_usage} -> {after_usage}')
            print('PASS original User startup text, XTalk ordering, long input, target compiler recovery and no-growth cleanup: '
                  f'{before_usage} -> {after_usage}', flush=True)
            raise SystemExit(0)
        if args.probe_user_startup:
            focus_graphical_terminal()
            command('zload /Tests/UserStartUp.ZC', 'zc: loaded /Tests/UserStartUp.ZC')
            command('zcall UserStartUpChecks', 'zc => 0x000000000000002a')
        visible_text = command('zcall BootstrapGuiTerminalTextPixelCount', 'zc => 0x')
        text_pixels = re.search(r'zc => 0x([0-9a-fA-F]+)', visible_text)
        if not text_pixels or int(text_pixels.group(1), 16) < 100:
            graphical_screen('boot-text-failure')
            raise RuntimeError('boot graphical terminal has no visible document text pixels:\n' + visible_text)
        print(f'PASS boot graphical terminal visible text pixels: {int(text_pixels.group(1), 16)}', flush=True)
        if args.probe_user_startup:
            for check in ('HookChecks', 'CapacityChecks', 'SleepFairnessChecks', 'FrameLockChecks'):
                command('zcall UserStartUp'+check, 'zc => 0x000000000000002a', timeout=30)
            before = command('zstatus', 'failed=0')
            for _ in range(3):
                command('zcall UserStartUpCapacityChecks', 'zc => 0x000000000000002a', timeout=30)
            # Source TaskEnd retires a task immediately but defers reclaiming
            # its heaps until the DYING_JIFFIES quarantine expires.
            time.sleep(0.5)
            after = command('zstatus', 'failed=0')
            heap = r'data=(\d+)/\d+ code=(\d+)/\d+ allocations=(\d+)'
            before_heap = tuple(map(int, re.search(heap, before).groups()))
            after_heap = tuple(map(int, re.search(heap, after).groups()))
            if (after_heap[0] > before_heap[0] or
                after_heap[1:] != before_heap[1:]):
                raise RuntimeError('repeated user terminal creation/teardown left managed allocations:\n'
                                   + before + '\n' + after)
            print('PASS HomeSys document startup, late-bound hooks, prompt readiness, task capacity and exact cleanup', flush=True)
            if args.probe_gui_keyboard:
                command('zcall UserStartUpOpenChecks', 'zc => 0x000000000000002a', timeout=30)
                move_graphical_pointer(600,360)
                # User() leaves its new task focused; type through real devices.
                typed = type_graphical_prompt_line(
                    'user-second-typing', 'Print("Second %d\\n",40+2);')
                require_graphical_caret(typed,'Print("Second %d\\n",40+2);')
                qmp_send_key('ret')
                require_graphical_text('user-second-answer','Second 42')
                qmp_send_key('up')
                require_graphical_text('user-second-history','> Print("Second')
                qmp_send_key('down')
                require_graphical_text('user-second-cleared','Second 42')
                focus_graphical_terminal()
                qmp_send_key('up')
                command('zc Print("history_isolated=%d\\n",BootstrapGuiTerminalState(bootstrap_gui_terminal_task)->history_len==user_startup_first_history_before);',
                        'history_isolated=1')
                # Up above intentionally recalled the primary terminal's
                # history; Down clears that recalled line before the fresh
                # visible-typing check below.
                qmp_send_key('down')
                typed = type_graphical_prompt_line(
                    'user-first-typing', 'Print("First %d\\n",40+2);',
                    prompt_required=False)
                require_graphical_caret(typed,'Print("First %d\\n",40+2);')
                qmp_send_key('ret')
                require_graphical_text('user-first-answer','First 42')
                command('zc Print("second_state=%d/%X z=%d win=%d,%d,%d,%d\\n",TaskValidate(user_startup_second),user_startup_second->display_flags,user_startup_second->win_z_num,user_startup_second->win_left,user_startup_second->win_right,user_startup_second->win_top,user_startup_second->win_bottom);',
                        'second_state=')
                command('zc Print("second_doc=%X/%X top=%d x=%d y=%d scroll=%d,%d\\n",user_startup_second->display_doc->flags,user_startup_second->display_doc->locked_flags,user_startup_second->display_doc->top_line_num,user_startup_second->display_doc->x,user_startup_second->display_doc->y,user_startup_second->scroll_x,user_startup_second->scroll_y);',
                        'second_doc=')
                screen = require_graphical_text('user-both-terminals','Second 42')
                target = ROOT/'build'/('user-startup-usb-graphical.ppm' if args.usb_input else 'user-startup-graphical.ppm')
                shutil.copyfile(graphical_path('user-both-terminals'),target)
                command('zcall UserStartUpCloseChecks', 'zc => 0x000000000000002a')
                focus_graphical_terminal()
                type_graphical_prompt_line('user-primary-recovery', '40+2;')
                qmp_send_key('ret'); require_graphical_text('user-primary-answer','=42')
                print('PASS independent visible typing, output, history and prompt recovery in two original User terminals', flush=True)
            raise SystemExit(0)
        if args.probe_user_command:
            command('zload /Tests/UserCommand.ZC', 'zc: loaded /Tests/UserCommand.ZC')
            for check in ('InputChecks','Checks','ErrorChecks','LoopChecks','TaskChecks','KillChecks'):
                command('zcall UserCommand'+check, 'zc => 0x000000000000002a', timeout=30)
            before = command('zstatus', 'failed=0')
            for check in ('ErrorChecks','KillChecks'):
                command('zcall UserCommand'+check, 'zc => 0x000000000000002a', timeout=30)
            after = command('zstatus', 'failed=0')
            heap = r'data=([0-9/]+) code=([0-9/]+) allocations=([0-9]+)'
            if re.search(heap,before).groups() != re.search(heap,after).groups():
                raise RuntimeError('command error/kill cleanup leaked managed memory')
            print('PASS original input loop, persistent options, exception tags and exact error/kill reclamation', flush=True)
            if args.probe_gui_keyboard:
                focus_graphical_terminal()
                qmp_send_text('40+2;')
                require_graphical_text('native-typing','40+2;')
                qmp_send_key('ret')
                require_graphical_text('native-answer','=42')
                qmp_send_text('Print("Native %d\\n",42);'); qmp_send_key('ret')
                frame = require_graphical_text('native-output','Native 42')
                if len(re.findall(r'Native 42',frame[3])) != 1:
                    raise RuntimeError('direct Print output was duplicated')
                qmp_send_text('I64 GuiNative(){'); qmp_send_key('ret')
                require_graphical_text('native-continuation','... ')
                qmp_send_text('return 42;}'); qmp_send_key('ret')
                qmp_send_text('Print("NativeFn %d\\n",GuiNative);'); qmp_send_key('ret')
                require_graphical_text('native-function','NativeFn 42')
                qmp_send_text('1.5+2.25;'); qmp_send_key('ret')
                require_graphical_text('native-float','ansf=3.75')
                qmp_send_text('Option(OPTf_DECIMAL_ONLY,ON);'); qmp_send_key('ret')
                qmp_send_text('Print("NativeOpts %d\\n",OptionGet(OPTf_DECIMAL_ONLY)*42);'); qmp_send_key('ret')
                require_graphical_text('native-options','NativeOpts 42')
                qmp_send_text('Option(OPTf_DECIMAL_ONLY,OFF);'); qmp_send_key('ret')
                qmp_send_text('zc Option(OPTf_DECIMAL_ONLY,ON);'); qmp_send_key('ret')
                qmp_send_text('Print("NativeLegacy %d\\n",OptionGet(OPTf_DECIMAL_ONLY)*42);'); qmp_send_key('ret')
                require_graphical_text('native-legacy-options','NativeLegacy 42')
                qmp_send_text('zc Option(OPTf_DECIMAL_ONLY,OFF);'); qmp_send_key('ret')
                qmp_send_text('I64 GuiCancelled(){'); qmp_send_key('ret')
                require_graphical_text('native-cancel-pending','... ')
                qmp_send_key('esc')
                qmp_send_text('Print("NativeCancel %d\\n",42);'); qmp_send_key('ret')
                require_graphical_text('native-cancelled','NativeCancel 42')
                qmp_send_text('I64 GuiBroken( {)'); qmp_send_key('ret')
                require_graphical_text('native-error',"Expected ')'")
                qmp_send_text('throw(42);'); qmp_send_key('ret')
                require_graphical_text('native-throw','Except: 0x000000000000002a')
                qmp_send_text('Print("NativeRecovery %d\\n",40+2);'); qmp_send_key('ret')
                require_graphical_text('native-recovery','NativeRecovery 42')
                desktop = command('zstatus','user_command: original_loop=1 statements=')
                if not re.search(r'user_command: original_loop=1 statements=[1-9][0-9]*',desktop):
                    raise RuntimeError('graphical commands did not use the original execution loop')
                print('PASS visible direct ZealC typing/output, multiline definitions, options, Escape, parser/runtime recovery', flush=True)
            raise SystemExit(0)
        if 'system_server: running=1 separate=1' not in desktop:
            raise RuntimeError('boot did not start a separate source System server:\n'+desktop)
        command('zload /Tests/SystemServer.ZC', 'zc: loaded /Tests/SystemServer.ZC')
        for check in ('Checks', 'ChildChecks', 'ErrorChecks', 'WaitChecks'):
            command('zcall SystemServer'+check, 'zc => 0x000000000000002a', timeout=20)
        print('PASS dedicated source System server, shared definitions, task ownership and recovery', flush=True)
        if args.probe_system_launch:
            command('zc Print("Direct %d",Sys("19+23;"));', 'Direct 42')
            command('zc Print("Formatted %d",Sys("%d+%d;",19,23));', 'Formatted 42')
        command('zload /Tests/SystemLaunch.ZC', 'zc: loaded /Tests/SystemLaunch.ZC')
        for check in ('QueueChecks','Checks','TaskChecks','FileChecks','PopupChecks','LeakChecks'):
            command('zcall SystemLaunch'+check, 'zc => 0x000000000000002a', timeout=20)
        launch_heap = r'data=([0-9/]+) code=([0-9/]+) allocations=([0-9]+)'
        launch_before = command('zstatus', 'failed=0')
        command('zcall SystemLaunchLeakChecks', 'zc => 0x000000000000002a')
        launch_after = command('zstatus', 'failed=0')
        if re.search(launch_heap, launch_before).groups() != re.search(launch_heap, launch_after).groups():
            raise RuntimeError('system requests leaked managed memory')
        print('PASS system requests complete and restore queues without arena growth', flush=True)
        if args.probe_system_launch or args.probe_system_server:
            if args.probe_gui_keyboard:
                focus_graphical_terminal()
                qmp_send_text('zload /Tests/SystemLaunch.ZC'); qmp_send_key('ret')
                require_graphical_text('launch-private-load','zc: loaded /Tests/SystemLaunch.ZC')
                for check,label in (('QueueChecks','Queue'), ('Checks','System'), ('TaskChecks','Caller'),
                                    ('FileChecks','SystemFile'), ('PopupChecks','Popup')):
                    qmp_send_text('zc Print("  '+label+' %d",SystemLaunch'+check+'());')
                    qmp_send_key('ret')
                    require_graphical_text('launch-private-'+check,label+' 42')
                if args.probe_system_server:
                    qmp_send_text('zload /Tests/SystemServer.ZC'); qmp_send_key('ret')
                    require_graphical_text('server-private-load','zc: loaded /Tests/SystemServer.ZC')
                    for check,label in (('Checks','Server'), ('ChildChecks','ServerChild'),
                                        ('ErrorChecks','ServerRecovery'), ('WaitChecks','ServerWait')):
                        qmp_send_text('zc Print("  '+label+' %d",SystemServer'+check+'());')
                        qmp_send_key('ret')
                        require_graphical_text('server-private-'+check,label+' 42')
            raise SystemExit(0)
        command('zload /Tests/TaskSettings.ZC', 'zc: loaded /Tests/TaskSettings.ZC')
        command('zcall TaskCurrentDirectoryChecks', 'zc => 0x000000000000002a')
        command('zcall TaskSettingsRestoreChecks', 'zc => 0x000000000000002a')
        command('zcall TaskSettingsDocumentChecks', 'zc => 0x000000000000002a')
        command('zload /Tests/SourceContext.ZC', 'zc: loaded /Tests/SourceContext.ZC')
        command('zcall SourceContextMaskChecks', 'zc => 0x000000000000002a')
        command('zcall SourceContextFailureChecks', 'zc => 0x000000000000002a')
        command('zcall SourceContextFindChecks', 'zc => 0x000000000000002a')
        command('zcall SourceContextTaskChecks', 'zc => 0x000000000000002a')
        command('zcall SourceContextEditorFailureChecks', 'zc => 0x000000000000002a')
        if args.pci:
            command('zcall SourceContextProjectChecks', 'zc => 0x000000000000002a')
            command('zcall SourceContextDocLinkChecks', 'zc => 0x000000000000002a')
        command('zstatus', 'failed=0')
        if args.probe_source_context:
            raise SystemExit(0)
        command('zload /Tests/HomePath.ZC', 'zc: loaded /Tests/HomePath.ZC')
        for check in ('ReadChecks', 'ContextChecks', 'TaskChecks'):
            command('zcall HomePath'+check, 'zc => 0x000000000000002a')
        if args.pci:
            command('zcall HomePathFileChecks', 'zc => 0x000000000000002a')
        if args.probe_home_path:
            before = command('zstatus', 'failed=0')
            for _ in range(3):
                for check in ('ReadChecks', 'ContextChecks', 'TaskChecks'):
                    command('zcall HomePath'+check, 'zc => 0x000000000000002a')
            after = command('zstatus', 'failed=0')
            heap = r'data=([0-9/]+) code=([0-9/]+) allocations=([0-9]+)'
            if re.search(heap, before).groups() != re.search(heap, after).groups():
                raise RuntimeError('HomeSet/context/child teardown leaked managed memory')
            print('PASS home aliases, original HomeSet, directory contexts and shared lifetime', flush=True)
            if args.probe_gui_keyboard:
                focus_graphical_terminal()
                text = 'Print("Home device %d\\n",HomePathReadChecks);'
                qmp_send_text(text)
                typed = require_graphical_text('home-path-typing', text)
                require_graphical_caret(typed, text)
                qmp_send_key('ret')
                require_graphical_text('home-path-answer', 'Home device 42')
            if args.pci:
                guest.terminate()
                try:
                    guest.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    guest.kill(); guest.wait()
                guest = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, bufsize=0)
                buf.clear()
                until(b'\n> ', timeout=180)
                command('zload /Tests/HomePath.ZC', 'zc: loaded /Tests/HomePath.ZC')
                command('zcall HomePathPersistentChecks', 'zc => 0x000000000000002a')
                if args.probe_gui_keyboard:
                    focus_graphical_terminal()
                    qmp_send_text('ExeFile("~/HomePathValue.ZC");')
                    qmp_send_key('ret')
                    require_graphical_text('home-path-reopen', 'ExeFile("~/HomePathValue.ZC");')
                    qmp_send_text('Print("Home reboot %d\\n",HomePathValue);')
                    qmp_send_key('ret')
                    require_graphical_text('home-path-reboot-answer', 'Home reboot 42')
                print('PASS home source save, relative include, task compiler and reboot/reopen', flush=True)
            raise SystemExit(0)
        if args.probe_task_settings:
            # Relative zload uses the caller's directory, while #include is
            # resolved from the loaded source's directory by the compiler.
            command('zcall TaskDirectoryFileChecks', 'zc => 0x000000000000002a')
            command('zc Cd("/Tests");', 'zc: executed')
            command('zload SettingsCwd.ZC', 'zc: loaded /Tests/SettingsCwd.ZC')
            command('zcall SettingsCwd', 'zc => 0x000000000000002a')
            command('zc #include "SettingsCwdValue.HH"', 'zc: executed')
            command('zc Cd("/");', 'zc: executed')
            raise SystemExit(0)
        command('zload /Tests/SourceExecute.ZC', 'zc: loaded /Tests/SourceExecute.ZC')
        command('zcall SourceFileExecutionChecks', 'zc => 0x000000000000002a')
        command('zcall SourceFileTaskChecks', 'zc => 0x000000000000002a')
        if args.pci:
            command('zcall SourceFileNativeChecks', 'zc => 0x000000000000002a')
            command('zcall SourceFileOverlayChecks', 'zc => 0x000000000000002a')
        command('zstatus', 'failed=0')
        if args.probe_source_execute:
            raise SystemExit(0)
        command('zload /Tests/CompilerRecovery.ZC', 'zc: loaded /Tests/CompilerRecovery.ZC')
        command('zc I64 RecoveryStable(){return 42;}', 'zc: executed')
        command('zc I64 RecoveryStable(){return MissingRecoveryName;}', 'current statement discarded')
        command('zcall RecoveryStable', 'zc => 0x000000000000002a')
        command('zc class CRecoveryHalf {I64 value; MissingRecoveryType member;};', 'current statement discarded')
        command('zc class CRecoveryHalf {I64 value;};', 'zc: executed')
        command('zc I64 RecoveryClassSize(){return sizeof(CRecoveryHalf)+34;}', 'zc: executed')
        command('zcall RecoveryClassSize', 'zc => 0x000000000000002a')
        command('zcall CompilerRecoveryChecks', 'zc => 0x000000000000002a')
        command('zcall CompilerRecoveryOperandChecks', 'zc => 0x000000000000002a')
        command('zcall CompilerRecoveryReentryChecks', 'zc => 0x000000000000002a')
        try:
            command('zcall CompilerRecoveryJobChecks', 'zc => 0x000000000000002a')
        except RuntimeError:
            command('zc Print("recovery_queue: code=%d flags=%X\\n",'
                    'Fs->server_ctrl.next_waiting->job_code,Fs->server_ctrl.next_waiting->flags);',
                    'recovery_queue:')
            raise
        command('zcall CompilerRecoveryTaskChecks', 'zc => 0x000000000000002a')
        command('zcall CompilerRecoveryStackChecks', 'zc => 0x000000000000002a')
        command('zcall CompilerRecoveryLeakChecks', 'zc => 0x000000000000002a')
        before_recovery = command('zstatus', 'failed=0')
        command('zcall CompilerRecoveryLeakChecks', 'zc => 0x000000000000002a')
        after_recovery = command('zstatus', 'failed=0')
        heap_pattern = r'data=([0-9/]+) code=([0-9/]+) allocations=([0-9]+)'
        if re.search(heap_pattern, before_recovery).groups() != re.search(heap_pattern, after_recovery).groups():
            raise RuntimeError('repeated compiler failures leaked managed memory')
        print('PASS compiler errors preserve existing definitions and have no arena growth', flush=True)
        rejected = command('zload /Tests/CompilerRecoveryFile.ZC', 'current statement discarded')
        if 'zc: loaded /Tests/CompilerRecoveryFile.ZC' in rejected:
            raise RuntimeError('failed source load was reported as successful')
        command('zcall RecoveryFileGood', 'zc => 0x000000000000002a')
        command('zc ExeFile("/Tests/CompilerRecoveryFile.ZC");', 'current statement discarded')
        command('zcall RecoveryFileGood', 'zc => 0x000000000000002a')
        recovered = command('zstatus', 'failed=0')
        if not re.search(r'compiler_recovery: errors=[1-9][0-9]* active=0', recovered):
            raise RuntimeError('compiler recovery diagnostics did not return to an inactive boundary')
        if args.probe_compiler_recovery:
            raise SystemExit(0)
        command('zload /Tests/CompilerContext.ZC', 'zc: loaded /Tests/CompilerContext.ZC')
        command('zcall CompilerContextChecks', 'zc => 0x000000000000002a')
        command('zcall CompilerContextTaskChecks', 'zc => 0x000000000000002a')
        command('zcall CompilerContextControlChecks', 'zc => 0x000000000000002a')
        command('zcall CompilerContextYieldChecks', 'zc => 0x000000000000002a')
        command('zcall CompilerContextRegChecks', 'zc => 0x000000000000002a')
        command('zcall CompilerContextLeakChecks', 'zc => 0x000000000000002a')
        before_context = command('zstatus', 'failed=0')
        command('zcall CompilerContextLeakChecks', 'zc => 0x000000000000002a')
        after_context = command('zstatus', 'failed=0')
        if re.search(heap_pattern, before_context).groups() != re.search(heap_pattern, after_context).groups():
            raise RuntimeError('compiler contexts leaked managed memory')
        print('PASS compiler contexts restore nested queues and options without arena growth', flush=True)
        command('zc Option(OPTf_DECIMAL_ONLY,ON);', 'zc: executed')
        command('zc 0x2a;', 'disabled by OPTf_DECIMAL_ONLY')
        command('zc Option(OPTf_DECIMAL_ONLY,OFF);', 'zc: executed')
        command('zc 0x2a;', 'zc: executed')
        command('zstatus', 'failed=0')
        if args.probe_compiler_context:
            raise SystemExit(0)
        command('zload /Tests/RunFile.ZC', 'zc: loaded /Tests/RunFile.ZC')
        command('zcall LastFunChecks', 'zc => 0x000000000000002a')
        command('zcall LastFunRawChecks', 'zc => 0x000000000000002a')
        command('zcall RunFileChecks', 'zc => 0x000000000000002a')
        command('zcall RunFileTaskChecks', 'zc => 0x000000000000002a')
        command('zcall RunFileNestedChecks', 'zc => 0x000000000000002a')
        command('zcall RunFileJobChecks', 'zc => 0x000000000000002a')
        command('zcall RunFileLeakChecks', 'zc => 0x000000000000002a')
        before_run_file = command('zstatus', 'failed=0')
        command('zcall RunFileLeakChecks', 'zc => 0x000000000000002a')
        after_run_file = command('zstatus', 'failed=0')
        if re.search(heap_pattern, before_run_file).groups() != re.search(heap_pattern, after_run_file).groups():
            raise RuntimeError('failed function definitions leaked last-function metadata')
        print('PASS LastFun/RunFile arguments, task isolation and failed-definition rollback', flush=True)
        if args.probe_run_file:
            raise SystemExit(0)
        command('zload /Tests/SourceExceptions.ZC', 'zc: loaded /Tests/SourceExceptions.ZC')
        for check in ('Checks', 'FileChecks', 'NestedChecks', 'JobChecks', 'TaskChecks', 'LeakChecks'):
            command('zcall SourceException'+check, 'zc => 0x000000000000002a')
        before_exceptions = command('zstatus', 'failed=0')
        command('zcall SourceExceptionLeakChecks', 'zc => 0x000000000000002a')
        after_exceptions = command('zstatus', 'failed=0')
        if re.search(heap_pattern, before_exceptions).groups() != re.search(heap_pattern, after_exceptions).groups():
            raise RuntimeError('source exception cleanup leaked managed memory')
        command('zc ExePutS2("I64 ExceptionHalf( {");', 'current statement discarded')
        for failing in ("zc throw('Prompt');", 'zcall SourceExceptionThrow',
                        'zload /Tests/SourceExceptionThrow.ZC'):
            command(failing, 'zc: uncaught guest exception 0x')
            command('zcall SourceExceptionValue', 'zc => 0x000000000000002a')
        before_commands = command('zstatus', 'failed=0')
        for _ in range(4):
            command('zload /Tests/SourceExceptionThrow.ZC', 'zc: uncaught guest exception 0x')
        after_commands = command('zstatus', 'failed=0')
        if re.search(heap_pattern, before_commands).groups() != re.search(heap_pattern, after_commands).groups():
            raise RuntimeError('failed public source commands leaked managed memory')
        command('zstatus', 'failed=0')
        print('PASS source/file throws restore compiler contexts and queues without arena growth', flush=True)
        if args.probe_source_exceptions:
            raise SystemExit(0)
        if args.probe_gui_keyboard:
            # The source manager opens its menu when the uninitialized QMP
            # pointer rests in the top screen row. Select the actual client
            # before checking its prompt or delivering keys. This workflow
            # has already emitted enough guest output to scroll the boot banner
            # off-screen, so use the live prompt as the readiness marker.
            focus_graphical_terminal()
            if args.probe_gui_workflow:
                # The grouped desktop bar now starts with File. Its two-row
                # submenu is narrower than the former all-in-one OS menu, so
                # scope the pixel check to that actual dropdown.
                move_graphical_pointer(4, 4)
                menu_deadline = time.monotonic() + 5
                menu_screen = graphical_screen('os-menu-top-edge')
                while time.monotonic() < menu_deadline:
                    blue_menu = rgb_pixels_in(menu_screen, (0, 0, 170),
                                               (0, 0, 240, 100))
                    yellow_menu = rgb_pixels_in(menu_screen, (255, 255, 85),
                                                 (0, 0, 240, 100))
                    # File contains Files and Disk, so only two submenu rows
                    # are drawn beneath its six-character menu column.
                    blue_submenu = rgb_pixels_in(menu_screen, (0, 0, 170),
                                                  (0, 8, 56, 32))
                    if (blue_menu >= 100 and yellow_menu >= 100 and
                            blue_submenu >= 500):
                        print('PASS grouped menu bar and File dropdown render on hover',
                              flush=True)
                        break
                    time.sleep(0.1)
                    menu_screen = graphical_screen('os-menu-top-edge')
                else:
                    raise RuntimeError('File menu did not appear on the desktop top edge; '
                                       f'see {graphical_path("os-menu-top-edge")} and .txt\n'
                                       + menu_screen[3])
                # Find the OS root from the live menu entry widths instead of
                # assuming it begins at the left edge as it did before roots
                # were grouped. Drag into Help and release to exercise the
                # actual MESSAGE_KEY_UP action route.
                os_menu_x = graphical_menu_root_x('OS')
                move_graphical_pointer(os_menu_x + 4, 4)
                qmp_command('input-send-event', {'events': [
                    {'type': 'btn', 'data': {'down': True, 'button': 'left'}}]})
                time.sleep(0.2)
                menu_title_held = graphical_screen('os-menu-title-held')
                press_status = command('zstatus', 'pointer_state: ready=1')
                press_match = re.search(
                    r'pointer_state: ready=1 pointer_xy=(\d+)/(\d+) buttons=(\d+)',
                    press_status)
                if (not press_match or int(press_match.group(3)) != 1 or
                        abs(int(press_match.group(1)) - (os_menu_x + 4)) > 1 or
                        abs(int(press_match.group(2)) - 4) > 1):
                    qmp_command('input-send-event', {'events': [
                        {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
                    raise RuntimeError('OS menu press did not reach the guest at its title row:\n' +
                                       press_status + menu_title_held[3])
                # The menu bar is one FONT_HEIGHT row tall; Help is the first
                # submenu item, so target the center of screen row 1.
                move_graphical_pointer(os_menu_x + 4, 12)
                time.sleep(0.2)
                help_row = graphical_screen('os-menu-help-held')
                held_status = command('zstatus', 'pointer_state: ready=1')
                held_match = re.search(
                    r'pointer_state: ready=1 pointer_xy=(\d+)/(\d+) buttons=(\d+)',
                    held_status)
                if (not held_match or int(held_match.group(3)) != 1 or
                        abs(int(held_match.group(1)) - (os_menu_x + 4)) > 1 or
                        abs(int(held_match.group(2)) - 12) > 1):
                    qmp_command('input-send-event', {'events': [
                        {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
                    raise RuntimeError('OS menu button was not held over the Help row:\n' +
                                       held_status + help_row[3])
                if rgb_pixels_in(help_row, (255, 255, 85),
                                 (os_menu_x, 8, os_menu_x + 56, 16)) < 32:
                    # Never leave QEMU's button held when this diagnostic
                    # exits; a failed hover/selection probe must not strand
                    # the interactive desktop in pointer capture.
                    qmp_command('input-send-event', {'events': [
                        {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
                    status = command('zstatus', 'usb_input:')
                    raise RuntimeError('OS menu Help row was not highlighted while held:\n' +
                                       status +
                                       'title-held frame:\n' + menu_title_held[3] +
                                       'help-held frame:\n' + help_row[3])
                qmp_command('input-send-event', {'events': [
                    {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
                menu_release_status = command('zstatus', 'buttons=0')
                pointer_match = re.search(r'pointer_xy=(\d+)/(\d+)\s+buttons=0',
                                           menu_release_status)
                if (not pointer_match or
                        abs(int(pointer_match.group(1)) - (os_menu_x + 4)) > 1 or
                        abs(int(pointer_match.group(2)) - 12) > 1):
                    raise RuntimeError('OS menu click did not release on the Help row:\n' +
                                       menu_release_status)
                menu_action_deadline = time.monotonic() + 5
                # The Help command list uses ZealOS's dim green, whose
                # luminance falls just below the default white-text cutoff.
                menu_action_screen = graphical_screen('os-menu-help-action', 90000)
                while True:
                    if 'Graphical commands:' in menu_action_screen[3]:
                        break
                    if time.monotonic() >= menu_action_deadline:
                        status = command('zstatus', 'usb_input:')
                        raise RuntimeError('OS menu click did not dispatch Help:\n' +
                                           status + menu_action_screen[3])
                    time.sleep(0.1)
                    menu_action_screen = graphical_screen('os-menu-help-action', 100000)
                print('PASS OS menu press-drag-release dispatched Help', flush=True)
                # Leaving the top row should collapse the highlight and
                # dropdown. This is the normal desktop state when the pointer
                # is elsewhere.
                move_graphical_pointer(400, 300)
                menu_deadline = time.monotonic() + 5
                menu_screen = graphical_screen('os-menu-left-top-row')
                while time.monotonic() < menu_deadline:
                    yellow_menu = rgb_pixels_in(menu_screen, (255, 255, 85),
                                                 (0, 0, 800, 8))
                    blue_submenu = rgb_pixels_in(menu_screen, (0, 0, 170),
                                                  (0, 8, 56, 32))
                    if yellow_menu < 100 and blue_submenu < 500:
                        print('PASS grouped menu highlight and dropdown hide off the top row',
                              flush=True)
                        break
                    time.sleep(0.1)
                    menu_screen = graphical_screen('os-menu-left-top-row')
                else:
                    raise RuntimeError('grouped menu stayed open after leaving the desktop top row; '
                                       f'see {graphical_path("os-menu-left-top-row")} and .txt\n'
                                       + menu_screen[3])
                # Add an isolated source-volume fixture before FileMgr builds
                # its tree. This joins the Files/editor checks into the same
                # user workflow and avoids editing any bundled boot source.
                command('zput Home/ZZFileMgrEdit.ZC I64 FileMgrOriginal(){return 17;}',
                        'zput: saved Home/ZZFileMgrEdit.ZC')
                # Files launches the native FileMgr over the merged
                # source-volume directory tree.
                move_graphical_pointer(4, 4)
                menu_deadline = time.monotonic() + 5
                menu_screen = graphical_screen('os-menu-files-open')
                while time.monotonic() < menu_deadline:
                    if (rgb_pixels_in(menu_screen, (0, 0, 170),
                                      (0, 0, 240, 100)) >= 100 and
                            rgb_pixels_in(menu_screen, (0, 0, 170),
                                          (0, 8, 56, 32)) >= 500):
                        break
                    time.sleep(0.1)
                    menu_screen = graphical_screen('os-menu-files-open')
                else:
                    raise RuntimeError('OS menu did not reopen for Files selection:\n' +
                                       menu_screen[3])
                qmp_command('input-send-event', {'events': [
                    {'type': 'btn', 'data': {'down': True, 'button': 'left'}}]})
                time.sleep(0.2)
                files_press_status = command('zstatus', 'pointer_state: ready=1')
                files_press_match = re.search(
                    r'pointer_state: ready=1 pointer_xy=(\d+)/(\d+) buttons=(\d+)',
                    files_press_status)
                if not files_press_match or int(files_press_match.group(3)) != 1:
                    qmp_command('input-send-event', {'events': [
                        {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
                    raise RuntimeError('OS menu Files press did not reach the guest:\n' +
                                       files_press_status)
                move_graphical_pointer(4, 12)
                time.sleep(0.2)
                files_row = graphical_screen('os-menu-files-held')
                files_held_status = command('zstatus', 'pointer_state: ready=1')
                files_held_match = re.search(
                    r'pointer_state: ready=1 pointer_xy=(\d+)/(\d+) buttons=(\d+)',
                    files_held_status)
                if (not files_held_match or
                        int(files_held_match.group(3)) != 1 or
                        abs(int(files_held_match.group(1)) - 4) > 1 or
                        abs(int(files_held_match.group(2)) - 12) > 1):
                    qmp_command('input-send-event', {'events': [
                        {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
                    raise RuntimeError('OS menu button was not held over Files:\n' +
                                       files_held_status + files_row[3])
                if rgb_pixels_in(files_row, (255, 255, 85), (0, 8, 56, 16)) < 32:
                    qmp_command('input-send-event', {'events': [
                        {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
                    raise RuntimeError('OS menu Files row was not highlighted while held:\n' +
                                       files_row[3])
                qmp_command('input-send-event', {'events': [
                    {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
                files_release_status = command('zstatus', 'buttons=0')
                files_pointer_match = re.search(
                    r'pointer_xy=(\d+)/(\d+)\s+buttons=0', files_release_status)
                if (not files_pointer_match or
                        abs(int(files_pointer_match.group(1)) - 4) > 1 or
                        abs(int(files_pointer_match.group(2)) - 12) > 1):
                    raise RuntimeError('OS menu Files click did not release on its row:\n' +
                                       files_release_status)
                # Move the guest cursor away from the first text cell before
                # OCR; the bitmap cursor otherwise obscures the section label.
                move_graphical_pointer(400, 300)
                time.sleep(0.2)
                files_view = graphical_screen('os-menu-files-action')
                if (rgb_pixels_in(files_view, (170, 0, 170),
                                  (0, 0, 400, 600)) < 1000 or
                        rgb_pixels_in(files_view, (85, 85, 255),
                                      (0, 0, 400, 600)) < 1000):
                    raise RuntimeError('Files did not show the native source-volume tree:\n' +
                                       files_view[3])
                print('PASS native FileMgr source title and directory tree rendered',
                      flush=True)
                # Open a source leaf through FileMgr's real pointer path.
                # Derive the rendered row from its DocEntry so the check
                # follows the same window/document hit-testing as a user.
                point_def = (
                    'zc I64 FileMgrSourcePoint(){CTask*t=bootstrap_gui_terminal_task;'
                    'CDoc*d=t->display_doc;CDocEntry*e=d->head.next;'
                    'for(;e!=d;e=e->next)if(e->type_u8==DOCT_MENU_VAL&&'
                    '!StrCompare(e->user_data(CDirEntry*)->full_name,'
                    '"/Home/ZZFileMgrEdit.ZC")){Print("fmxy=%d,%d\\n",'
                    't->pix_left+t->scroll_x+(e->x-d->line_start_col)*8+4,'
                    't->pix_top+t->scroll_y+(e->y-d->top_line_num)*8+4);'
                    'return 42;}return 0;}'
                )
                command(point_def, 'zc: executed')
                point_result = command('zcall FileMgrSourcePoint',
                                       'zc => 0x000000000000002a')
                point_match = re.search(r'fmxy=(\d+),(\d+)', point_result)
                if not point_match:
                    raise RuntimeError('could not locate the FileMgr edit fixture:\n' +
                                       point_result)
                file_x, file_y = map(int, point_match.groups())
                if not (0 <= file_x < 800 and 0 <= file_y < 600):
                    raise RuntimeError(f'FileMgr edit fixture is outside the guest display: '
                                       f'{file_x},{file_y}')
                move_graphical_pointer(file_x, file_y)
                graphical_mouse_button('left', True)
                time.sleep(0.15)
                graphical_mouse_button('left', False)
                selected_name = 'ZZFileMgrEdit.ZC'
                file_click_deadline = time.monotonic() + 5
                opened_file = ''
                opened_file_screen = None
                while time.monotonic() < file_click_deadline:
                    opened_file_screen = graphical_screen(
                        'os-menu-files-click-open')
                    opened_file = opened_file_screen[3]
                    if (selected_name.lower() in opened_file.lower() and
                            'filemgroriginal' in opened_file.lower() and
                            'return 17' in opened_file.lower()):
                        break
                    time.sleep(0.1)
                else:
                    raise RuntimeError('click did not open the source file in the graphical editor:\n' +
                                       f'click={file_x},{file_y}\n' + opened_file)
                qmp_send_keys(['shift', 'esc'])
                filemgr_restore_deadline = time.monotonic() + 5
                restored_screen = None
                while time.monotonic() < filemgr_restore_deadline:
                    restored_screen = graphical_screen(
                        'os-menu-files-click-return')
                    if ('filemgroriginal' not in restored_screen[3].lower() and
                            rgb_pixels_in(restored_screen, (85, 85, 255),
                                          (0, 0, 400, 200)) >= 500):
                        break
                    time.sleep(0.1)
                else:
                    raise RuntimeError('Shift+Escape did not return from the editor to FileMgr:\n' +
                                       (restored_screen[3] if restored_screen else ''))
                # Open the temporary source from the native tree, edit it in
                # the upstream DolDoc editor, and prove Escape saves through
                # DocWrite into the same persistent source volume.
                filemgr_target = command(
                    'zc Print("filemgr_target=%s\\n", '
                    'bootstrap_gui_terminal_task->display_doc->cur_entry->'
                    'user_data(CDirEntry *)->full_name);',
                    'filemgr_target=')
                if 'filemgr_target=/Home/ZZFileMgrEdit.ZC' not in filemgr_target:
                    raise RuntimeError('FileMgr did not preserve the selected edit fixture:\n' +
                                       filemgr_target)
                qmp_send_key('spc')
                filemgr_edit_deadline = time.monotonic() + 5
                filemgr_edit_screen = None
                while time.monotonic() < filemgr_edit_deadline:
                    filemgr_edit_screen = graphical_screen('filemgr-source-edit-open')
                    if ('filemgroriginal' in filemgr_edit_screen[3].lower() and
                            'zzfilemgredit.zc' in filemgr_edit_screen[3].lower()):
                        break
                    time.sleep(0.1)
                else:
                    raise RuntimeError('Space did not open the temporary source from FileMgr:\n' +
                                       (filemgr_edit_screen[3] if filemgr_edit_screen else ''))
                qmp_send_keys(['shift', 'esc'])
                filemgr_return_deadline = time.monotonic() + 5
                while time.monotonic() < filemgr_return_deadline:
                    filemgr_return_screen = graphical_screen(
                        'filemgr-source-edit-return')
                    if ('filemgroriginal' not in filemgr_return_screen[3].lower() and
                            rgb_pixels_in(filemgr_return_screen, (85, 85, 255),
                                          (0, 0, 400, 200)) >= 500):
                        break
                    time.sleep(0.1)
                else:
                    raise RuntimeError('Shift+Escape did not return from the FileMgr editor:\n' +
                                       filemgr_return_screen[3])
                print(f'PASS native FileMgr opened {selected_name} by click and Space, '
                      'returning with Shift+Escape', flush=True)
                qmp_send_key('esc')
                files_restore_deadline = time.monotonic() + 5
                while True:
                    files_restored = command('zstatus', 'desktop:')
                    if 'keyboard_focus=Terminal' in files_restored:
                        break
                    if time.monotonic() >= files_restore_deadline:
                        raise RuntimeError('Escape did not close the Files window and restore '
                                           'terminal focus:\n' + files_restored)
                    time.sleep(0.1)
                restored_screen = graphical_screen('os-menu-files-restored')
                if rgb_pixels_in(restored_screen, (85, 85, 255),
                                 (0, 0, 400, 200)) >= 500:
                    raise RuntimeError('Files window remained visible after Escape:\n' +
                                       restored_screen[3])
                print('PASS OS menu Files opened native FileMgr and Escape restored the terminal',
                      flush=True)
                command('zrm Home/ZZFileMgrEdit.ZC',
                        'zrm: deleted Home/ZZFileMgrEdit.ZC')
                # Exercise Window Demo through the grouped Windows root using
                # the same press-drag-release path a user takes.
                select_os_menu_row('os-menu-windowdemo', 'Windows', 0)
                demo_deadline = time.monotonic() + 5
                demo_status = ''
                while True:
                    demo_status = command('zstatus', 'desktop:')
                    if 'windowdemo=1' in demo_status:
                        break
                    if time.monotonic() >= demo_deadline:
                        raise RuntimeError('OS menu Window Demo action did not start the demo:\n' +
                                           demo_status)
                    time.sleep(0.1)
                require_graphical_text('os-menu-windowdemo-action', 'WINDOW A', timeout=8)
                command('windowdemostop',
                        'windowdemo: stopped; bootstrap shell display resumed')
                restored = command('zstatus',
                                   'desktop: winmgr=1 gterm=1 windowdemo=0 presented_frames=')
                if 'keyboard_focus=Terminal' not in restored:
                    raise RuntimeError('OS menu Window Demo did not restore terminal focus:\n' +
                                       restored)
                print('PASS OS menu press-drag-release launched Window Demo and restored the terminal',
                      flush=True)
                # The same desktop menu must launch the two graphics apps,
                # not merely leave them reachable through typed commands.
                for demo, row_index in (('latticeplay', 0), ('stockplay', 1)):
                    select_os_menu_row('os-menu-'+demo, 'Apps', row_index)
                    require_graphical_text(demo+'-menu-live-hud', 'Layer:', timeout=10,
                                           foreground_threshold=215000,
                                           scanline_offset=0)
                    qmp_send_key('esc')
                    require_graphical_text(demo+'-menu-returns', demo+' ok', timeout=10)
                    status = command('zstatus', 'failed=0')
                    if 'keyboard_focus=Terminal' not in status:
                        raise RuntimeError(f'OS menu {demo} did not return focus to the terminal:\n'+status)
                    print(f'PASS OS menu launched {demo}, rendered its HUD, and returned on Escape',
                          flush=True)
            require_graphical_text('boot-terminal', '> ')
            raw_pattern = r'usb_input:.* key_events=(\d+)' if args.usb_input else r'virtio_keyboard: ready=1 key_events=(\d+)'
            initial_input = command('zcall BootstrapGuiTerminalInputDiag', 'zc => 0x')
            initial_value = re.search(r'zc => 0x([0-9a-fA-F]+)', initial_input)
            if not initial_value:
                raise RuntimeError('could not inspect graphical input before typing:\n' +
                                   initial_input)
            initial_length = int(initial_value.group(1), 16) & 0xff
            if initial_length:
                print(f'INFO clearing {initial_length} stale graphical input characters',
                      flush=True)
            # A prior Escape/focus transition can leave a partially edited
            # command line when QEMU xHCI delivers its final key edge late.
            # Down is the shell's documented clear-line action; start the
            # visible-key assertions from a known empty prompt.
            qmp_send_key('down')
            clear_deadline = time.monotonic() + 3
            while True:
                input_diag = command('zcall BootstrapGuiTerminalInputDiag',
                                     'zc => 0x')
                input_value = re.search(r'zc => 0x([0-9a-fA-F]+)', input_diag)
                if input_value and (int(input_value.group(1), 16) & 0xff) == 0:
                    break
                if time.monotonic() >= clear_deadline:
                    raise RuntimeError('graphical prompt line did not clear before typing:\n' +
                                       input_diag)
                time.sleep(0.05)
            desktop = command('zstatus', 'desktop:')
            raw_before = re.search(raw_pattern, desktop)
            if not raw_before:
                raise RuntimeError('could not read low-level virtio keyboard event count:\n' + desktop)
            before = command('zcall BootstrapGuiTerminalKeyEventCount', 'zc => 0x')
            before_count = re.search(r'zc => 0x([0-9a-fA-F]+)', before)
            if not before_count:
                raise RuntimeError('could not read graphical-terminal key-event count:\n' + before)
            for input_length, key in enumerate(('h', 'e', 'l', 'p'), 1):
                qmp_send_key(key)
                input_deadline = time.monotonic() + 3
                while True:
                    input_diag = command('zcall BootstrapGuiTerminalInputDiag',
                                         'zc => 0x')
                    input_value = re.search(r'zc => 0x([0-9a-fA-F]+)', input_diag)
                    last_char_diag = command('zcall BootstrapGuiTerminalLastKeyChar',
                                             'zc => 0x')
                    last_char_value = re.search(r'zc => 0x([0-9a-fA-F]+)',
                                                last_char_diag)
                    if not input_value or not last_char_value:
                        raise RuntimeError('could not inspect graphical input while typing:\n'
                                           + input_diag + last_char_diag)
                    current_length = int(input_value.group(1), 16) & 0xff
                    current_char = int(last_char_value.group(1), 16)
                    if current_length == input_length and current_char == ord(key):
                        break
                    if current_length > input_length or time.monotonic() >= input_deadline:
                        status = command('zstatus', 'gui_keyboard:')
                        raise RuntimeError(f'graphical input diverged at {key!r}: '
                                           f'length={current_length} '
                                           f'last_char=0x{current_char:x}\n{status}')
                    time.sleep(0.05)
            time.sleep(0.3)
            typing = require_graphical_prompt_text('typing-before-return', 'help')
            require_graphical_pointer()
            require_graphical_caret(typing, '> help')
            input_diag = command('zcall BootstrapGuiTerminalInputDiag', 'zc => 0x')
            input_value = re.search(r'zc => 0x([0-9a-fA-F]+)', input_diag)
            if not input_value or int(input_value.group(1), 16) & 0xff != 4:
                status = command('zstatus', 'gui_keyboard:')
                raise RuntimeError('graphical prompt did not contain exactly "help" before Return:\n'
                                   + input_diag + status)
            qmp_send_key('ret')
            time.sleep(0.5)
            after = command('zcall BootstrapGuiTerminalKeyEventCount', 'zc => 0x')
            after_count = re.search(r'zc => 0x([0-9a-fA-F]+)', after)
            if not after_count or int(after_count.group(1), 16) < int(before_count.group(1), 16) + 5:
                raise RuntimeError('printable virtio keys did not reach the boot graphical terminal:\n' + after)
            raw_after = command('zstatus', keyboard_diagnostic)
            raw_count = re.search(raw_pattern, raw_after)
            if not raw_count or int(raw_count.group(1)) < int(raw_before.group(1)) + 5:
                raise RuntimeError('QMP keys did not reach the virtio keyboard device:\n' + raw_after)
            command('zcall BootstrapGuiTerminalLastCommandIsHelp',
                    'zc => 0x0000000000000001')
            if args.usb_input:
                # A second input backend must not turn "help" into doubled
                # letters or add extra key-downs at the OS boundary.
                if int(after_count.group(1), 16) != int(before_count.group(1), 16) + 5:
                    raise RuntimeError('USB input published duplicate/missing key-downs:\n'+after)
                if 'errors=0' not in raw_after or not re.search(r'pointer_reports=[1-9]\d*', raw_after):
                    raise RuntimeError('USB pointer reports/errors failed:\n'+raw_after)
                print('PASS USB press/release delivery and exclusive keyboard/pointer ownership', flush=True)
                for button, mask in (('left', 1), ('right', 2), ('middle', 4)):
                    qmp_command('input-send-event', {'events': [
                        {'type': 'btn', 'data': {'down': True, 'button': button}}]})
                    time.sleep(0.15)
                    command('zstatus', f'buttons={mask}')
                    qmp_command('input-send-event', {'events': [
                        {'type': 'btn', 'data': {'down': False, 'button': button}}]})
                    time.sleep(0.15)
                    command('zstatus', 'buttons=0')
                qmp_command('input-send-event', {'events': [
                    {'type': 'btn', 'data': {'down': True, 'button': 'wheel-up'}}]})
                qmp_command('input-send-event', {'events': [
                    {'type': 'btn', 'data': {'down': False, 'button': 'wheel-up'}}]})
                time.sleep(0.15)
                wheel_status = command('zstatus', 'usb_input:')
                if not re.search(r'wheel_events=[1-9]\d*', wheel_status):
                    raise RuntimeError('native USB wheel report was not decoded:\n'+wheel_status)
                print('PASS native USB left/right/middle press/release and wheel', flush=True)
                # Pointer/button checks run outside the terminal. The source
                # manager correctly focuses its background there; select the
                # application again before asserting application typing.
                focus_graphical_terminal()
                fast_command = 'zc I64 UsbFastTyping(){return 42;}'
                qmp_send_text(fast_command, delay=0.1)
                require_graphical_prompt_text('usb-fast-typing', fast_command)
                require_graphical_caret(graphical_screen('usb-fast-typing'), '> '+fast_command)
                qmp_send_key('ret')
                require_graphical_text('usb-fast-submit', 'zc: executed')
                # Definitions belong to the graphical task's compiler scope.
                # Invoke it there rather than from the independent UART task.
                qmp_send_text('zcall UsbFastTyping')
                qmp_send_key('ret')
                require_graphical_text('usb-fast-run', 'zc => 0x000000000000002a')
                print('PASS native USB mixed-case/modifier typing at ten characters per second', flush=True)
                # Run geometry manipulation after key-injection checks so a
                # resize/reflow cannot affect the keyboard acceptance probe.
                probe_graphical_terminal_resize()
            last_key = command('zcall BootstrapGuiTerminalLastKeyChar', 'zc => 0x')
            last_key_value = re.search(r'zc => 0x([0-9a-fA-F]+)', last_key)
            if not last_key_value or int(last_key_value.group(1), 16) != 10:
                raise RuntimeError('graphical terminal did not record the Return key after help:\n' + last_key)
            before_zls = command('zcall BootstrapGuiTerminalKeyEventCount', 'zc => 0x')
            before_zls_count = re.search(r'zc => 0x([0-9a-fA-F]+)', before_zls)
            if not before_zls_count:
                raise RuntimeError('could not read graphical-terminal key count before zls:\n' + before_zls)
            for key in ('z', 'l', 's', 'ret'):
                qmp_send_key(key)
            time.sleep(0.5)
            after_zls = command('zcall BootstrapGuiTerminalKeyEventCount', 'zc => 0x')
            after_zls_count = re.search(r'zc => 0x([0-9a-fA-F]+)', after_zls)
            if (not after_zls_count or
                    int(after_zls_count.group(1), 16) < int(before_zls_count.group(1), 16) + 4):
                raise RuntimeError('printable virtio keys did not reach the graphical terminal for zls:\n' + after_zls)
            command('zcall BootstrapGuiTerminalLastCommandIsZls',
                    'zc => 0x0000000000000001')
            command('zcall BootstrapGuiShellSmoke',
                    'zc => 0x000000000000002a')
            zls_last_key = command('zcall BootstrapGuiTerminalLastKeyChar', 'zc => 0x')
            zls_last_key_value = re.search(r'zc => 0x([0-9a-fA-F]+)', zls_last_key)
            if not zls_last_key_value or int(zls_last_key_value.group(1), 16) != 10:
                raise RuntimeError('graphical terminal did not record Return after zls:\n' + zls_last_key)
            before_edit = command('zcall BootstrapGuiTerminalKeyEventCount', 'zc => 0x')
            before_edit_count = re.search(r'zc => 0x([0-9a-fA-F]+)', before_edit)
            if not before_edit_count:
                raise RuntimeError('could not read graphical-terminal key count before editing test:\n' + before_edit)
            # Type "zvl", move left once, and insert "o" to form zvol.
            for key in ('z', 'v', 'l', 'left', 'o', 'ret'):
                qmp_send_key(key)
            time.sleep(0.5)
            after_edit = command('zcall BootstrapGuiTerminalKeyEventCount', 'zc => 0x')
            after_edit_count = re.search(r'zc => 0x([0-9a-fA-F]+)', after_edit)
            if (not after_edit_count or
                    int(after_edit_count.group(1), 16) < int(before_edit_count.group(1), 16) + 6):
                raise RuntimeError('graphical terminal did not receive cursor-editing keys:\n' + after_edit)
            command('zcall BootstrapGuiTerminalLastCommandIsZvol',
                    'zc => 0x0000000000000001')
            before_history = command('zcall BootstrapGuiTerminalKeyEventCount', 'zc => 0x')
            before_history_count = re.search(r'zc => 0x([0-9a-fA-F]+)', before_history)
            if not before_history_count:
                raise RuntimeError('could not read graphical-terminal key count before history test:\n' + before_history)
            for key in ('up', 'ret'):
                qmp_send_key(key)
            time.sleep(0.5)
            after_history = command('zcall BootstrapGuiTerminalKeyEventCount', 'zc => 0x')
            after_history_count = re.search(r'zc => 0x([0-9a-fA-F]+)', after_history)
            if (not after_history_count or
                    int(after_history_count.group(1), 16) < int(before_history_count.group(1), 16) + 2):
                raise RuntimeError('graphical terminal did not receive command-history keys:\n' + after_history)
            command('zcall BootstrapGuiTerminalLastCommandIsZvol',
                    'zc => 0x0000000000000001')
            print('PASS QMP printable, cursor-editing, and history keys formed and submitted "help", "zls", and "zvol" in the graphical terminal', flush=True)
            command('zc I64 GuiEditorFileFindCheck(){return FileFind("/System/DolDoc/DocEd.ZC",NULL,1024)*42;}',
                    'zc: executed')
            command('zcall GuiEditorFileFindCheck',
                    'zc => 0x000000000000002a')
            command('zc I64 GuiEditorDocReadCheck(){CDoc *d=DocRead("/System/DolDoc/DocEd.ZC");I64 r=1;if(!d)return r;if(d->doc_signature==DOC_SIGNATURE_VAL)r=42;DocDel(d);return r;}',
                    'zc: executed')
            command('zcall GuiEditorDocReadCheck',
                    'zc => 0x000000000000002a')
            command('zc I64 GuiEditorDocFileEdCheck(){I64 n=1;if(DocFileEd(LK_FILE,"/System/DolDoc/DocEd.ZC",NULL,&n,EDF_BAIL|EDf_BAIL))return 42;return 1;}',
                    'zc: executed')
            command('zcall GuiEditorDocFileEdCheck',
                    'zc => 0x000000000000002a')
            # The full edit/save/reboot/reopen workflow below exercises Ed
            # from the graphical shell. Calling Ed() directly from this
            # serial-side probe can leave the GUI terminal focus elsewhere,
            # so keep this section limited to the bundled document API checks.
            print('PASS bundled DocEd source is present, readable, and accepted by DocFileEd', flush=True)
            # Exercise the manager transition from the same graphical task
            # that owns the keyboard. This catches context checks that pass
            # from the serial shell but reject a valid GUI app task.
            qmp_send_text('windowdemo')
            qmp_send_key('ret')
            time.sleep(0.5)
            demo_status = command('zstatus',
                                  'desktop: winmgr=1 gterm=1 windowdemo=1 presented_frames=')
            if 'failed=0' not in demo_status:
                raise RuntimeError('graphical window demo left the native compiler failed:\n' + demo_status)
            command('windowdemostop',
                    'windowdemo: stopped; bootstrap shell display resumed')
            restored = command('zstatus',
                               'desktop: winmgr=1 gterm=1 windowdemo=0 presented_frames=')
            if 'keyboard_focus=Terminal' not in restored:
                raise RuntimeError('graphical terminal did not recover keyboard focus after windowdemo:\n' + restored)
            # Stopping Window Demo restores the shell display, but the first
            # keyboard edge can race the manager's final focus handoff. Click
            # the terminal again and wait for a visibly assembled command
            # before submitting it.
            focus_graphical_terminal()
            type_graphical_prompt_line('windowdemo-poststop-zls', 'zls')
            qmp_send_key('ret')
            command('zcall BootstrapGuiTerminalLastCommandIsZls',
                    'zc => 0x0000000000000001')
            print('PASS QMP graphical window demo launch/stop restored the terminal and keyboard focus', flush=True)
            if args.probe_gui_workflow:
                command('zc compiler_recovery_committed=0;', 'zc: executed')
                option_command = 'zc Option(OPTf_DECIMAL_ONLY,ON);'
                qmp_send_text(option_command)
                # QMP acknowledges a key edge before the cooperative GUI task
                # necessarily consumes it. Let the guest scheduler run until
                # the full visible input buffer catches up, with a bounded
                # timeout so a genuinely lost character still fails loudly.
                input_deadline = time.monotonic() + 2
                input_state = ''
                input_len = input_result = -1
                while True:
                    input_state = command('zcall BootstrapGuiTerminalInputDiag', 'zc => 0x')
                    input_match = re.search(r'zc => 0x([0-9a-fA-F]+)', input_state)
                    input_diag = int(input_match.group(1), 16) if input_match else -1
                    input_len = input_diag & 255
                    input_result = (input_diag >> 8) & 255
                    if input_len == len(option_command) and input_result == 1:
                        break
                    if time.monotonic() >= input_deadline:
                        break
                    time.sleep(0.1)
                print(f'INFO live GUI input buffer: length={input_len} last_edit={input_result}', flush=True)
                if input_len != len(option_command) or input_result != 1:
                    raise RuntimeError('USB keys did not assemble the expected live GUI command buffer:\n' + input_state)
                require_graphical_text('option-command-echo', '> ' + option_command)
                qmp_send_key('ret')
                qmp_send_text('zc compiler_recovery_committed=0x2a;')
                qmp_send_key('ret')
                option_error = require_graphical_text('option-error', 'Hex constant disabled by')
                require_graphical_text('option-error-option-name', 'OPTf_DECIMAL_ONLY.')
                if 'current statement discarded' not in option_error[3]:
                    raise RuntimeError('lexer error hid its recovery diagnostic in the graphical terminal')
                command('zcall CompilerRecoveryResult', 'zc => 0x0000000000000000')
                qmp_send_text('zc Option(OPTf_DECIMAL_ONLY,OFF);')
                qmp_send_key('ret')
                qmp_send_text('zc compiler_recovery_committed=0x2a;')
                qmp_send_key('ret')
                require_graphical_text('option-restored', 'zc compiler_recovery_committed=0x2a;')
                command('zcall CompilerRecoveryResult', 'zc => 0x000000000000002a')
                print('PASS graphical prompt retains compiler options across commands and recovered errors', flush=True)
                command('zc compiler_recovery_committed=0;', 'zc: executed')
                qmp_send_text('zc I64 GuiRecoveryBroken( {')
                qmp_send_key('ret')
                require_graphical_text('compiler-error', 'current statement discarded')
                qmp_send_text('zc compiler_recovery_committed=42;')
                qmp_send_key('ret')
                require_graphical_text('compiler-recovered', 'zc compiler_recovery_committed=42;')
                command('zcall CompilerRecoveryResult', 'zc => 0x000000000000002a')
                resumed_desktop = command('zstatus', 'failed=0')
                if 'keyboard_focus=Terminal' not in resumed_desktop or 'gterm=1' not in resumed_desktop:
                    raise RuntimeError('compiler error lost the graphical terminal or its keyboard focus')
                print('PASS graphical compiler error returned to the prompt and accepted a corrected command', flush=True)
                executable = ('I64 GuiSourceFile(){Print("Native file executed");return 42;} '
                              'source_file_gui_result=GuiSourceFile()*(Fs!=sys_task);')
                command('zput Tests/GuiExecute.ZC '+executable,
                        'zput: saved Tests/GuiExecute.ZC')
                qmp_send_text('zc ExeFile("/Tests/GuiExecute.ZC");')
                qmp_send_key('ret')
                executed = require_graphical_text('native-file-executed',
                                                  'Native file executed')
                if executed[3].count('Native file executed') != 1:
                    raise RuntimeError('source-file output was duplicated in the graphical terminal:\n'+executed[3])
                command('zcall SourceFileGuiResult', 'zc => 0x000000000000002a')
                print('PASS upstream ExeFile compiled and ran in the graphical terminal task', flush=True)
                app_source = ('I64 GuiRunArgs(I64 a,I64 b){'
                              'run_file_gui_result=(a+b)*(Fs!=sys_task);'
                              'Print("Native args %d",run_file_gui_result);'
                              'return run_file_gui_result;}')
                command('zput Tests/GuiRunArgs.ZC '+app_source,
                        'zput: saved Tests/GuiRunArgs.ZC')
                qmp_send_text('zc RunFile("/Tests/GuiRunArgs",0,19,23);')
                qmp_send_key('ret')
                app_output = require_graphical_text('native-run-file-args', 'Native args 42')
                if app_output[3].count('Native args 42') != 1:
                    raise RuntimeError('RunFile output was duplicated in the graphical terminal')
                command('zcall RunFileGuiResult', 'zc => 0x000000000000002a')
                print('PASS unchanged RunFile forwarded arguments and ran in the graphical terminal task', flush=True)
                # Load private definitions into the GUI task, rather than
                # calling root-owned fixtures inherited through its hash chain.
                qmp_send_text('zload /Tests/RunFile.ZC')
                qmp_send_key('ret')
                require_graphical_text('run-file-private-load', 'zc: loaded /Tests/RunFile.ZC')
                result_line = 'zc => 0x000000000000002a'
                before_task = graphical_screen('run-file-before-private-task')[3].count(result_line)
                task_check_command = 'zcall RunFileTaskChecks'
                qmp_send_text(task_check_command)
                require_graphical_prompt_text('run-file-private-command',
                                              task_check_command)
                qmp_send_key('ret')
                private_task = require_graphical_text('run-file-private-task', result_line)
                if private_task[3].count(result_line) != before_task + 1:
                    raise RuntimeError('graphical task did not complete its private child-function check')
                print('PASS graphical task-owned code spawns an owned child and keeps LastFun task-local', flush=True)
                qmp_send_text('zload /Tests/SystemLaunch.ZC')
                qmp_send_key('ret')
                require_graphical_text('system-private-load', 'zc: loaded /Tests/SystemLaunch.ZC')
                for check,label in (('QueueChecks','Queue'), ('Checks','System'), ('TaskChecks','Caller'),
                                    ('FileChecks','SystemFile'), ('PopupChecks','Popup')):
                    run_graphical_terminal_command(
                        'system-private-'+check, 'zcall SystemLaunch'+check,
                        result_line)
                print('PASS graphical task-owned system/file/popup launch and caller recovery', flush=True)
                qmp_send_text('zload /Tests/SystemServer.ZC'); qmp_send_key('ret')
                require_graphical_text('server-private-load', 'zc: loaded /Tests/SystemServer.ZC')
                for check,label in (('Checks','Server'), ('ChildChecks','ServerChild'),
                                    ('ErrorChecks','ServerRecovery'), ('WaitChecks','ServerWait')):
                    run_graphical_terminal_command(
                        'server-private-'+check, 'zcall SystemServer'+check,
                        result_line)
                print('PASS graphical System server, child-owned source and asynchronous request lifetime', flush=True)
                qmp_send_text('zload /Tests/SourceExceptions.ZC')
                qmp_send_key('ret')
                require_graphical_text('exceptions-private-load', 'zc: loaded /Tests/SourceExceptions.ZC')
                for check in ('Checks', 'NestedChecks', 'TaskChecks'):
                    run_graphical_terminal_command(
                        'exceptions-private-'+check,
                        'zcall SourceException'+check, result_line)
                run_graphical_terminal_command(
                    'exceptions-uncaught', 'zcall SourceExceptionThrow',
                    'zc: uncaught guest exception 0x')
                run_graphical_terminal_command(
                    'exceptions-prompt-recovered',
                    'zcall SourceExceptionValue', result_line)
                command('zstatus', 'failed=0')
                print('PASS graphical source/file throws clean up and accept the next command', flush=True)
                editor_path = '/Tests/GuiBringup.ZC'
                source = 'I64 GuiBringup(){return 42;}'
                editor_open_command = 'edit '+editor_path
                type_graphical_prompt_line('gui-bringup-editor-command',
                                           editor_open_command)
                qmp_send_key('ret')
                time.sleep(0.5)
                command('zstatus', 'gui_editor_active=1')
                move_graphical_pointer(200, 200)
                graphical_mouse_button('left', True)
                time.sleep(0.15)
                graphical_mouse_button('left', False)
                qmp_send_text(source)
                require_graphical_text('editor-typed-source', source)
                qmp_send_key('esc')
                time.sleep(0.5)
                command('zstatus', 'gui_editor_active=0')
                load_saved_command = 'zload disk:Tests/GuiBringup.ZC'
                type_graphical_prompt_line('gui-bringup-load-command',
                                           load_saved_command)
                qmp_send_key('ret')
                require_graphical_text('editor-saved-load',
                                       'zc: loaded disk:Tests/GuiBringup.ZC')
                qmp_send_text('zcall GuiBringup')
                qmp_send_key('ret')
                require_graphical_text('editor-saved-run',
                                       'zc => 0x000000000000002a')
                guest.terminate()
                try:
                    guest.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    guest.kill()
                    guest.wait()
                guest = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                                         stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, bufsize=0)
                buf.clear()
                until(b'\n> ')
                # A fresh QEMU pointer starts at (0,0), opening the guest's
                # top-row menu over the terminal. Restore client focus before
                # asserting prompt glyphs, just as on the initial boot.
                # The serial prompt can precede the GUI task's startup frame,
                # so wait for its actual banner before targeting its window.
                wait_for_graphical_terminal_banner()
                focus_graphical_terminal()
                type_graphical_prompt_line('editor-reopen-command',
                                           'edit '+editor_path)
                qmp_send_key('ret')
                require_graphical_text('editor-reopened-after-reboot', source)
                command('zstatus', 'gui_editor_active=1')
                qmp_send_keys(['shift', 'esc'])
                time.sleep(0.5)
                command('zstatus', 'gui_editor_active=0')
                qmp_send_text('help')
                qmp_send_key('ret')
                cancelled = require_graphical_text(
                    'editor-cancelled-after-reboot',
                    'Graphical commands:')
                if 'edit: could not open document' in cancelled[3]:
                    raise RuntimeError('cancelling an open editor reported an open failure')
                print('PASS graphical edit/save/compile/run/reboot/reopen workflow', flush=True)
                # The live Lattice demos used to be exposed only through the
                # serial command dispatcher. Exercise the graphical shell
                # command route too, and prove each live loop returns to the
                # same terminal after Escape.
                for demo in ('latticeplay', 'stockplay'):
                    qmp_send_text(demo)
                    qmp_send_key('ret')
                    # GrPrint draws HUD text in LTGRAY (224), while DolDoc's
                    # scanout text is brighter. Use a matching threshold and
                    # scan from row zero because GrPrint has no cell offset.
                    require_graphical_text(demo+'-live-hud', 'Layer:', timeout=10,
                                           foreground_threshold=215000,
                                           scanline_offset=0)
                    if demo == 'latticeplay':
                        exercise_live_lattice_controls()
                    qmp_send_key('esc')
                    require_graphical_text(demo+'-returns-to-terminal',
                                           demo+' ok', timeout=10)
                    status = command('zstatus', 'failed=0')
                    if 'gterm=1' not in status:
                        raise RuntimeError(f'{demo} did not restore the graphical terminal:\n'+status)
                    print(f'PASS graphical {demo} launch, live HUD, and Escape recovery',
                          flush=True)
            if args.usb_input:
                final_usb = command('zstatus', 'usb_input:')
                if not re.search(r'usb_input:.* errors=0', final_usb):
                    raise RuntimeError('USB transfer errors after graphical workflow:\n'+final_usb)
                print('PASS native USB transfer/event ring cycling without errors', flush=True)
                if args.usb_with_virtio or args.usb_hotplug:
                    dma_before = int(re.search(r'dma_bytes=(\d+)', final_usb).group(1))
                    if args.usb_hotplug:
                        qmp_command('input-send-event', {'events': [
                            {'type': 'key', 'data': {'down': True,
                             'key': {'type': 'qcode', 'data': 'shift'}}}]})
                        time.sleep(0.15)
                    qmp_command('device_del', {'id': 'usb-kbd'})
                    time.sleep(0.3)
                    command('zstatus', 'keyboard=virtio/serial')
                    if args.usb_hotplug and args.usb_with_virtio:
                        qmp_command('input-send-event', {'events': [
                            {'type': 'key', 'data': {'down': False,
                             'key': {'type': 'qcode', 'data': 'shift'}}}]})
                    if args.usb_with_virtio:
                        qmp_send_text('help')
                        require_graphical_text('virtio-fallback-typing', '> help')
                        qmp_send_key('ret')
                        time.sleep(0.3)
                        command('zcall BootstrapGuiTerminalLastCommandIsHelp',
                                'zc => 0x0000000000000001')
                        print('PASS unplugged USB keyboard returns input to VirtIO', flush=True)
                if args.usb_hotplug:
                    def wait_usb(expected, devices=None, hubs=None):
                        deadline = time.monotonic() + 8
                        while True:
                            status = command('zstatus', 'usb_input:')
                            good = expected in status
                            if devices is not None:
                                good &= f'controllers=1 devices={devices} ' in status
                            if hubs is not None:
                                good &= f'usb_topology: hubs={hubs} ' in status
                            if good:
                                if not re.search(r'usb_input:.* errors=0', status) or 'quarantined=0' not in status:
                                    raise RuntimeError('USB reconnect failed or quarantined DMA:\n'+status)
                                return status
                            if time.monotonic() >= deadline:
                                raise RuntimeError('USB topology did not settle:\n'+status)
                            time.sleep(0.15)
                    def add_input(driver, identity, port):
                        qmp_command('device_add', {'driver': driver, 'id': identity,
                                                 'bus': 'usb.0', 'port': usb_prefix+str(port)})
                    count = (3 if args.usb_pointer == 'tablet' else 2) + args.usb_hubs
                    # More reconnects than device records. Neither slots nor
                    # reserved DMA may leak, and held Shift must be released.
                    for cycle in range(12):
                        add_input('usb-kbd', 'usb-kbd', 1)
                        status = wait_usb('keyboard=usb', devices=count)
                        if cycle == 0 and not args.usb_with_virtio:
                            # No host keyboard handler exists during USB-only
                            # unplug; deliver physical release after reattach.
                            qmp_command('input-send-event', {'events': [
                                {'type': 'key', 'data': {'down': False,
                                 'key': {'type': 'qcode', 'data': 'shift'}}}]})
                        if int(re.search(r'dma_bytes=(\d+)', status).group(1)) != dma_before:
                            raise RuntimeError('USB keyboard reconnect leaked DMA memory:\n'+status)
                        if cycle in (0, 11):
                            qmp_send_text('help', delay=0.1)
                            require_graphical_text(f'usb-reconnect-{cycle}', '> help')
                            qmp_send_key('ret')
                            time.sleep(0.2)
                            command('zcall BootstrapGuiTerminalLastCommandIsHelp', 'zc => 0x0000000000000001')
                        if cycle != 11:
                            qmp_command('device_del', {'id': 'usb-kbd'})
                            wait_usb('keyboard=virtio/serial', devices=count-1)
                    print('PASS 12 keyboard reconnects reuse DMA and restore visible typing after held-Shift unplug', flush=True)
                    if args.usb_pointer == 'tablet':
                        qmp_command('input-send-event', {'events': [
                            {'type': 'btn', 'data': {'down': True, 'button': 'left'}}]})
                        time.sleep(0.15)
                        command('zstatus', 'buttons=1')
                        qmp_command('device_del', {'id': 'usb-tablet'})
                        wait_usb('pointer=usb-mouse', devices=count-1)
                        command('zstatus', 'buttons=0')
                        qmp_command('input-send-event', {'events': [
                            {'type': 'btn', 'data': {'down': False, 'button': 'left'}}]})
                        add_input('usb-tablet', 'usb-tablet', 3)
                        wait_usb('pointer=usb-tablet', devices=count)
                        require_graphical_pointer()
                        focus_graphical_terminal()
                        print('PASS tablet unplug releases held button, selects mouse, and reattach restores visible pointer', flush=True)
                    if args.usb_hubs:
                        # QEMU removes descendants when their hub is unrealized.
                        qmp_command('device_del', {'id': 'usb-hub0'})
                        wait_usb('keyboard=virtio/serial', devices=0, hubs=0)
                        for tier in range(args.usb_hubs):
                            qmp_command('device_add', {'driver': 'usb-hub',
                                'id': f'usb-hub{tier}', 'bus': 'usb.0', 'port': '1.'*tier+'1'})
                        add_input('usb-kbd', 'usb-kbd', 1)
                        add_input('usb-mouse', 'usb-mouse', 2)
                        if args.usb_pointer == 'tablet':
                            add_input('usb-tablet', 'usb-tablet', 3)
                        status = wait_usb('keyboard=usb', devices=count, hubs=args.usb_hubs)
                        if int(re.search(r'dma_bytes=(\d+)', status).group(1)) != dma_before:
                            raise RuntimeError('USB hub tree reconnect leaked DMA memory:\n'+status)
                        qmp_send_text('help')
                        require_graphical_text('usb-hub-tree-reconnect', '> help')
                        qmp_send_key('ret')
                        time.sleep(0.2)
                        command('zcall BootstrapGuiTerminalLastCommandIsHelp', 'zc => 0x0000000000000001')
                        print('PASS hub subtree removal/reattach disables descendants and restores native typing without DMA growth', flush=True)
                # Verify the three source WinTile routines against both live
                # terminal tasks after keyboard and caret checks. Exercise the
                # first through the visible menu; call the other two actions
                # directly because tiling moves the menu's owning terminal.
                main_task = 'bootstrap_gui_terminal_task'
                second_task = 'bootstrap_gui_terminal_home_second'
                second_valid = command('zc Print("gterm_second_valid=%d\\n", '
                                       'TaskValidate(bootstrap_gui_terminal_home_second));',
                                       'gterm_second_valid=')
                if 'gterm_second_valid=1' not in second_valid:
                    raise RuntimeError('OS menu tiling needs the second terminal task:\n' +
                                       second_valid)
                original_main = graphical_task_rect(main_task)
                original_second = graphical_task_rect(second_task)
                def restore_tiled_terminals():
                    command(f'zc WinHorz({original_main[0]},{original_main[1]},{main_task}); '
                            f'WinVert({original_main[2]},{original_main[3]},{main_task}); '
                            f'WinHorz({original_second[0]},{original_second[1]},{second_task}); '
                            f'WinVert({original_second[2]},{original_second[3]},{second_task});',
                            'zc: executed')
                def check_tiled_pair(label):
                    main_rect = graphical_task_rect(main_task)
                    second_rect = graphical_task_rect(second_task)
                    left, right, top, bottom = main_rect
                    other_left, other_right, other_top, other_bottom = second_rect
                    if label == 'horizontal':
                        ordered = sorted((main_rect, second_rect), key=lambda r: r[2])
                        if (main_rect[0:2] != second_rect[0:2] or
                                ordered[0][3] >= ordered[1][2]):
                            raise RuntimeError('Tile Windows Horizontally did not create '
                                               f'top/bottom bands: {main_rect}, {second_rect}')
                    elif label == 'vertical':
                        ordered = sorted((main_rect, second_rect), key=lambda r: r[0])
                        if (main_rect[2:4] != second_rect[2:4] or
                                ordered[0][1] >= ordered[1][0]):
                            raise RuntimeError('Tile Windows Vertically did not create '
                                               f'left/right columns: {main_rect}, {second_rect}')
                    else:
                        overlap_x = max(0, min(right, other_right) -
                                        max(left, other_left) + 1)
                        overlap_y = max(0, min(bottom, other_bottom) -
                                        max(top, other_top) + 1)
                        if (overlap_x and overlap_y) or right-left > 52 or bottom-top > 39:
                            raise RuntimeError('Tile Windows as Grid did not create '
                                               f'bounded, non-overlapping cells: '
                                               f'{main_rect}, {second_rect}')
                    print(f'PASS OS menu tile {label}: {main_rect}, {second_rect}',
                          flush=True)
                try:
                    select_os_menu_row('os-menu-tile-horizontal', 'Windows', 1)
                    check_tiled_pair('horizontal')
                    restore_tiled_terminals()
                    for label, action in (('vertical', 7), ('grid', 8)):
                        command(f'zc BootstrapGuiTerminalMenuAction({action});',
                                'zc: executed')
                        check_tiled_pair(label)
                        restore_tiled_terminals()
                finally:
                    restore_tiled_terminals()
            raise SystemExit(0)
        if args.probe_gui_terminal:
            command('zcall BootstrapGuiTerminalOutputOwnershipChecks',
                    'zc => 0x000000000000002a')
        command('zcall BootstrapGuiTerminalSmoke',
                'zc => 0x000000000000002a')
        command('zcall BootstrapGuiShellSmoke',
                'zc => 0x000000000000002a')
        command('zcall BootstrapCtrlWheelZoomChecks',
                'zc => 0x000000000000002a')
        if args.probe_gui_terminal:
            command('gtermstop', 'gterm: graphical terminal stopped')
            command('gterm',
                    'gterm: graphical terminal started; click it to type; Ctrl+C stops it')
            command('zcall BootstrapGuiTerminalOutputOwnershipChecks',
                    'zc => 0x000000000000002a')
            command('zcall BootstrapGuiTerminalSmoke',
                    'zc => 0x000000000000002a')
            raise SystemExit(0)
        command('zcall BootstrapSpriteOpcodeChecks',
                'zc => 0x000000000000002a')
        command('zcall BootstrapMat4x4Checks',
                'zc => 0x000000000000002a')
        command('zcall BootstrapCanUseUpstreamGrUpdateTasks',
                'zc => 0x0000000000000001')
        command('zcall BootstrapGrContextFlagsChecks',
                'zc => 0x000000000000002a')
        command('zc I64 GrSourceTaskFrameChecks(){return bootstrap_upstream_gr_update_tasks>0;}',
                'zc: executed')
        command('winmgr',
                'winmgr: upstream Window Manager running; shell remains on serial')
        time.sleep(.1)
        command('zcall GrSourceTaskFrameChecks',
                'zc => 0x0000000000000001')
        command('winmgr',
                'winmgr: upstream Window Manager running; shell remains on serial')
        command('winmgrstop',
                'winmgr: stopped; bootstrap framebuffer pump resumed')
        command('winmgr',
                'winmgr: upstream Window Manager running; shell remains on serial')
        command('winmgrstop',
                'winmgr: stopped; bootstrap framebuffer pump resumed')
        command('zcall BootstrapGrPackedTablesChecks',
                'zc => 0x000000000000002a')
        command('windowdemo',
                'windowdemo: source WinMgr running with two windows and a CDC control')
        command('zcall WindowDragLiveQueueDrainChecks',
                'zc => 0x000000000000002a')
        command('zcall WindowDragLiveCursorNoDocChecks',
                'zc => 0x000000000000002a')
        # Keep the manager alive across its first idle-driven redraw. The
        # start command can succeed even if a later task/window callback
        # still reaches an unresolved ZealC external.
        time.sleep(.1)
        result = command('zstatus', 'failed=0')
        if 'unresolved external call' in result:
            raise RuntimeError('windowdemo idle redraw hit an unresolved external call:\n'+result)
        command('zcall WindowDragLiveBasicRenderChecks',
                'zc => 0x000000000000002a')
        command('zcall WindowDragLiveRenderChecks',
                'zc => 0x000000000000002a')
        if args.window_frame_only:
            command('windowdemostop',
                    'windowdemo: stopped; bootstrap shell display resumed')
            raise SystemExit(0)
        command('windowdemostop',
                'windowdemo: stopped; bootstrap shell display resumed')
        command('zload /Tests/CallNOnStack.ZC',
                'zc: loaded /Tests/CallNOnStack.ZC')
        command('zcall CallNOnStackChecks',
                'zc => 0x000000000000002a')
        command('zreset', 'zc: native compiler ready')
        command('zvol', 'zvol: source partition ready' if args.pci else
                         'zvol: source partition unavailable')
        if args.pci:
            if not args.probe_source and not args.probe_upstream:
                command('zvol', 'bank=A generation=0')
            command('zls', 'Kernel/KernelA.HH bytes=113880')
        else:
            command('zls', 'zls: native source partition unavailable')
        if args.probe_source:
            for i, source in enumerate(args.probe_source):
                if args.probe_source_kernel:
                    command('zload /Kernel/KernelA.HH',
                            'zc: loaded /Kernel/KernelA.HH')
                    command('zload /Kernel/KernelB.HH',
                            'zc: loaded /Kernel/KernelB.HH')
                    command('zload /Kernel/KMathB.ZC',
                            'zc: loaded /Kernel/KMathB.ZC', timeout=30)
                    command('zload /Kernel/KernelC.HH',
                            'zc: loaded /Kernel/KernelC.HH', timeout=30)
                    command('zload /System/TaskBridge.ZC',
                            'zc: loaded /System/TaskBridge.ZC', timeout=30)
                guest.stdin.write(f'zload disk:Probe/Source{i}.ZC\r'.encode()); guest.stdin.flush()
                print('SOURCE PROBE '+str(source)+'\n'+until(b'\n> '), flush=True)
                if args.probe_source_call:
                    command('zcall '+args.probe_source_call,
                            'zc => 0x000000000000002a')
                command('zreset', 'native compiler ready')
            raise SystemExit(0)
        if args.probe_module == '/System/Win.ZC' and args.probe_winmgr:
            command('zload /Tests/NestedF64Member.ZC',
                    'zc: loaded /Tests/NestedF64Member.ZC')
            command('zcall NestedF64MemberChecks',
                    'zc => 0x000000000000002a')
            command('zload /Tests/DeferredExternUse.ZC',
                    'zc: loaded /Tests/DeferredExternUse.ZC')
            command('zload /Tests/DeferredExternDef.ZC',
                    'zc: loaded /Tests/DeferredExternDef.ZC')
            command('zcall DeferredExternCaller',
                    'zc => 0x000000000000002a')
            command('zload /Tests/GlobalExternDef.ZC',
                    'zc: loaded /Tests/GlobalExternDef.ZC')
            command('zload /Tests/GlobalExternUse.ZC',
                    'zc: loaded /Tests/GlobalExternUse.ZC')
            command('zcall GlobalExternLinkChecks',
                    'zc => 0x000000000000002a')
        if args.probe_window_buttons:
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Kernel/SerialDev/Message.ZC',
                    'zc: loaded /Kernel/SerialDev/Message.ZC')
            command('zload /Kernel/Job.ZC', 'zc: loaded /Kernel/Job.ZC')
            command('zload /Kernel/KeyDev.ZC', 'zc: loaded /Kernel/KeyDev.ZC')
            command('zload /Tests/WindowDragLive.ZC',
                    'zc: loaded /Tests/WindowDragLive.ZC')
            command('zcall WindowDragLiveButtonChecks',
                    'zc => 0x0000000000000000')
            result = command('zcall WindowDragLiveStart',
                             'zc => 0x000000000000002a')
            if 'unresolved external call' in result:
                raise RuntimeError('window redraw hit an unresolved external call:\n'+result)
            # windowdemo's first redraw occurs on a later idle frame, after
            # WindowDragLiveStart has returned to the shell. Exercise that
            # same path before the immediate render/stop checks can mask it.
            time.sleep(.1)
            result = command('zstatus', 'failed=0')
            if 'unresolved external call' in result:
                raise RuntimeError('background window redraw hit an unresolved external call:\n'+result)
            command('zcall WindowDragLiveRenderChecks',
                    'zc => 0x000000000000002a')
            command('zcall WindowDragLiveStop',
                    'zc => 0x0000000000000000')
            raise SystemExit(0)
        if args.probe_module:
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            guest.stdin.write(b'zload /Kernel/KernelB.HH\r')
            guest.stdin.flush()
            kb = until(b'\n> ')
            print('KERNELB LOAD\n'+kb, flush=True)
            if 'zc: loaded /Kernel/KernelB.HH' not in kb:
                raise RuntimeError('KernelB.HH did not load')
            if args.probe_module.startswith('/System/DolDoc/'):
                # Match normal graphics startup before probing DolDoc. Menu
                # and document rendering consume these graphics symbols.
                command('zload /Kernel/KernelC.HH',
                        'zc: loaded /Kernel/KernelC.HH')
                command('zload /Kernel/KMathB.ZC',
                        'zc: loaded /Kernel/KMathB.ZC', timeout=30)
                command('zload /System/Externs.ZC',
                        'zc: loaded /System/Externs.ZC')
                command('zload /System/Gr/GrInitA.ZC',
                        'zc: loaded /System/Gr/GrInitA.ZC')
                command('zload /System/Gr/GrGlobals.ZC',
                        'zc: loaded /System/Gr/GrGlobals.ZC')
                command('zload /System/Gr/Gr.HH',
                        'zc: loaded /System/Gr/Gr.HH')
                command('zload /System/Gr/SpriteBridge.ZC',
                        'zc: loaded /System/Gr/SpriteBridge.ZC')
                for graphics_module in ('GrExterns', 'GrPalette', 'GrDC',
                                        'GrBitMap'):
                    command(f'zload /System/Gr/{graphics_module}.ZC',
                            f'zc: loaded /System/Gr/{graphics_module}.ZC')
                command('zload /System/Menu.ZC',
                        'zc: loaded /System/Menu.ZC', timeout=15)
                command('zload /System/Win.ZC',
                        'zc: loaded /System/Win.ZC', timeout=30)
                command('zload /System/WinMgr.ZC',
                        'zc: loaded /System/WinMgr.ZC', timeout=30)
                command('zload /System/Gr/GrScreen.ZC',
                        'zc: loaded /System/Gr/GrScreen.ZC', timeout=30)
                command('zload /System/Gr/GrBootBridge.ZC',
                        'zc: loaded /System/Gr/GrBootBridge.ZC', timeout=30)
            if args.probe_module == '/System/Math/MathODE.ZC':
                # MathODE's MP task declarations (MPInt) are in KernelC, and
                # its default math imports come from KMathB just as at boot.
                command('zload /Kernel/KernelC.HH',
                        'zc: loaded /Kernel/KernelC.HH')
                command('zload /Kernel/KMathB.ZC',
                        'zc: loaded /Kernel/KMathB.ZC', timeout=30)
            if args.probe_module == '/Kernel/KTask.ZC':
                # KTask uses task and job declarations from KernelC and
                # Job.ZC. The probe supplies a compile-only scheduler stub
                # because loading Sched.ZC requires hardware-only externals.
                command('zload /Kernel/KernelC.HH',
                        'zc: loaded /Kernel/KernelC.HH')
                command('zload /Kernel/Job.ZC',
                        'zc: loaded /Kernel/Job.ZC', timeout=30)
                command('zload /Tests/KTaskProbeStubs.ZC',
                        'zc: loaded /Tests/KTaskProbeStubs.ZC')
            if args.probe_module == '/System/Win.ZC':
                command('zload /Tests/TaskOdeRegistry.ZC',
                        'zc: loaded /Tests/TaskOdeRegistry.ZC')
                command('zcall TaskOdeRegistryChecks',
                        'zc => 0x000000000000002a')
                command('zload /Tests/ExpCheck.ZC',
                        'zc: loaded /Tests/ExpCheck.ZC')
                command('zcall ExpCheck',
                        'zc => 0x000000000000002a')
                command('zload /Tests/LowPassCheck.ZC',
                        'zc: loaded /Tests/LowPassCheck.ZC')
                command('zcall LowPassCheck',
                        'zc => 0x000000000000002a')
                command('zload /Tests/RegDecl.ZC',
                        'zc: loaded /Tests/RegDecl.ZC')
                command('zcall RegDeclChecks',
                        'zc => 0x000000000000002a')
                command('zload /Tests/HashDefineBridge.ZC',
                        'zc: loaded /Tests/HashDefineBridge.ZC')
                command('zcall HashDefineBridgeChecks',
                        'zc => 0x000000000000002a')
            if args.probe_module == '/System/Win.ZC':
                command('zload /Kernel/KernelC.HH',
                        'zc: loaded /Kernel/KernelC.HH')
                # KernelC redeclares Spawn with a callback function-pointer
                # parameter. Keep the task bridge binding across that ABI match.
                command('zload /Tests/SpawnBridge.ZC',
                        'zc: loaded /Tests/SpawnBridge.ZC')
                command('zcall SpawnBridgeChecks',
                        'zc => 0x000000000000002a')
                # KernelC.HH declares native SysTry(start_label, skip_label).
                # Guest try/catch must keep using its private bridge ABI after
                # that declaration enters the compiler symbol table.
                command('zload /Tests/TaskExceptions.ZC',
                        'zc: loaded /Tests/TaskExceptions.ZC')
                command('zcall TaskExceptionChecks',
                        'zc => 0x000000000000002a')
                command('zload /Tests/GrSqrtCheck.ZC',
                        'zc: loaded /Tests/GrSqrtCheck.ZC')
                command('zcall GrSqrtCheck',
                        'zc => 0x0000000000000001')
                command('zload /Tests/WinMouseState.ZC',
                        'zc: loaded /Tests/WinMouseState.ZC')
                command('zcall WinMousePointerStateCheck',
                        'zc => 0x000000000000002a')
                command('zcall WinMouseHardStateCheck',
                        'zc => 0x000000000000002a')
                command('zcall WinCAllocCheck',
                        'zc => 0x0000000000000001')
                command('zcall WinTimeCheck',
                        'zc => 0x0000000000000001')
                command('zcall WinF64Check',
                        'zc => 0x0000000000000001')
                command('zcall WinF64ClockCheck',
                        'zc => 0x0000000000000001')
                command('zload /Tests/WinMgrParseProbe.ZC',
                        'zc: loaded /Tests/WinMgrParseProbe.ZC', timeout=15)
                command('zcall WinMgrParseProbe',
                        'zc => 0x000000000000002a', timeout=15)
                command('zload /Tests/LastClass.ZC',
                        'zc: loaded /Tests/LastClass.ZC')
                command('zcall LastClassChecks',
                        'zc => 0x000000000000002a')
                command('zload /System/Externs.ZC',
                        'zc: loaded /System/Externs.ZC')
                command('zload /System/Gr/GrInitA.ZC',
                        'zc: loaded /System/Gr/GrInitA.ZC')
                command('zload /System/Gr/Gr.HH',
                        'zc: loaded /System/Gr/Gr.HH')
                command('zload /System/Gr/SpriteBridge.ZC',
                        'zc: loaded /System/Gr/SpriteBridge.ZC')
                command('zload /System/Gr/GrExterns.ZC',
                        'zc: loaded /System/Gr/GrExterns.ZC')
                command('zload /System/Gr/GrGlobals.ZC',
                        'zc: loaded /System/Gr/GrGlobals.ZC')
                command('zload /Tests/GrGlobalsProbe.ZC',
                        'zc: loaded /Tests/GrGlobalsProbe.ZC')
                command('zcall GrGlobalsProbe',
                        'zc => 0x000000000000002a')
                # TaskBridge provides the ARM text-plane implementations for
                # the x86-assembly-only GrTextBase/GrAsm modules.
                command('zload /System/Gr/GrPalette.ZC',
                        'zc: loaded /System/Gr/GrPalette.ZC')
                command('zload /System/Gr/GrDC.ZC',
                        'zc: loaded /System/Gr/GrDC.ZC', timeout=30)
                # The bootstrap pixel path uses upstream RandU16 and
                # DistSqrI64 for probability dithering and nearest-point
                # tracking, so load their source bindings before plotting.
                command('zload /Kernel/KMathB.ZC',
                        'zc: loaded /Kernel/KMathB.ZC', timeout=30)
                command('zload /Tests/GrSpriteBridge.ZC',
                        'zc: loaded /Tests/GrSpriteBridge.ZC')
                command('zload /Tests/GrPlotSemantics.ZC',
                        'zc: loaded /Tests/GrPlotSemantics.ZC')
                command('zcall GrPlotSemanticsChecks',
                        'zc => 0x000000000000002a')
                # This focused probe is about manager-frame MathODE execution.
                # The ordinary sprite pixel checks run in the regular graphics
                # target; keep them out of this path so unrelated framebuffer
                # assertions cannot prevent the manager integration check.
                if not args.probe_live_winmgr_ode:
                    for check in ('GrDC2SpriteChecks',
                                  'GrSpriteEllipseChecks',
                                  'GrSpriteDitherChecks',
                                  'GrSpriteThickChecks',
                                  'GrSpriteArrowChecks',
                                  'GrSpritePolygonChecks',
                                  'GrSpriteRotatedRectChecks',
                                  'GrSpriteBitmapChecks'):
                        command(f'zcall {check}',
                                'zc => 0x000000000000002a')
                    command('zload /Tests/GrSpriteArrowSymmetry.ZC',
                            'zc: loaded /Tests/GrSpriteArrowSymmetry.ZC')
                    command('zcall GrSpriteArrowSymmetryChecks',
                            'zc => 0x000000000000002a')
                if not args.probe_live_winmgr_ode:
                    command('zcall GrSpriteCustomPaletteChecks',
                            'zc => 0x000000000000002a')
                command('zload /System/Gr/GrBitMap.ZC',
                        'zc: loaded /System/Gr/GrBitMap.ZC', timeout=30)
                command('zload /System/Gr/GrBlot3.ZC',
                        'zc: loaded /System/Gr/GrBlot3.ZC', timeout=30)
                command('zload /Tests/GrBlot3.ZC',
                        'zc: loaded /Tests/GrBlot3.ZC')
                command('zcall GrBlot3Checks',
                        'zc => 0x000000000000002a', timeout=30)
                command('zload /Tests/GrLineCdc.ZC',
                        'zc: loaded /Tests/GrLineCdc.ZC')
                command('zcall GrLineCdcChecks',
                        'zc => 0x000000000000002a')
                command('zload /Tests/GrDCLifecycle.ZC',
                        'zc: loaded /Tests/GrDCLifecycle.ZC')
                command('zcall GrDCExtClipChecks',
                        'zc => 0x000000000000002a')
                command('zload /Tests/GrSpriteText.ZC',
                        'zc: loaded /Tests/GrSpriteText.ZC')
                if not args.probe_live_winmgr_ode:
                    command('zcall GrSpriteTextChecks',
                            'zc => 0x000000000000002a')
                command('zload /Tests/GrSpriteSpline.ZC',
                        'zc: loaded /Tests/GrSpriteSpline.ZC')
                if not args.probe_live_winmgr_ode:
                    command('zcall GrSpriteSplineChecks',
                            'zc => 0x000000000000002a')
                command('zload /Tests/GrSpriteFloodFill.ZC',
                        'zc: loaded /Tests/GrSpriteFloodFill.ZC')
                if not args.probe_live_winmgr_ode:
                    command('zcall GrSpriteFloodFillChecks',
                            'zc => 0x000000000000002a')
                command('zload /Tests/GrSpriteSymmetry.ZC',
                        'zc: loaded /Tests/GrSpriteSymmetry.ZC')
                if not args.probe_live_winmgr_ode:
                    command('zcall GrSpriteSymmetryChecks',
                            'zc => 0x000000000000002a')
                    command('zcall SpriteScreenDepthChecks',
                            'zc => 0x000000000000002a')
                command('zload /Tests/GrSpriteTransform.ZC',
                        'zc: loaded /Tests/GrSpriteTransform.ZC')
                if not args.probe_live_winmgr_ode:
                    command('zcall GrSpriteTransformChecks',
                            'zc => 0x000000000000002a')
                command('zcall GrDCLifecycleChecks',
                        'zc => 0x000000000000002a')
                command('zcall GrDCUnalignedChecks',
                        'zc => 0x000000000000002a')
                command('zcall GrDCCopyChecks',
                        'zc => 0x000000000000002a')
                command('zcall GrDCDiffChecks',
                        'zc => 0x000000000000002a')
                command('zcall GrDCRoundTripChecks',
                        'zc => 0x000000000000002a')
                command('zcall GrDCCustomPaletteRoundTripChecks',
                        'zc => 0x000000000000002a')
                command('zcall GrDCPixelOpsChecks',
                        'zc => 0x000000000000002a')
                command('zcall GrDCTransformChecks',
                        'zc => 0x000000000000002a')
                command('zcall GrGlobalsProbe',
                        'zc => 0x000000000000002a')
            if args.probe_module == '/System/DolDoc/DocRecalc.ZC':
                command('zload /Kernel/SerialDev/Message.ZC',
                        'zc: loaded /Kernel/SerialDev/Message.ZC')
                command('zload /Kernel/Job.ZC', 'zc: loaded /Kernel/Job.ZC')
                command('zload /Kernel/KeyDev.ZC', 'zc: loaded /Kernel/KeyDev.ZC')
                for module in ('DocBin', 'DocNew', 'DocPutKey', 'DocPutS',
                               'DocDblBuf', 'DocTerm'):
                    command(f'zload /System/DolDoc/{module}.ZC',
                            f'zc: loaded /System/DolDoc/{module}.ZC')
                command('zcall BootstrapDefinesLoad',
                        'zc => 0x0000000000000000')
                for module in ('DocInit', 'DocPlain', 'DocRecalcLib', 'DocHighlight'):
                    command(f'zload /System/DolDoc/{module}.ZC',
                            f'zc: loaded /System/DolDoc/{module}.ZC')
            if args.probe_module == '/Kernel/Job.ZC':
                # TaskText Spawns InputFilterTask from SerialDev/Message.ZC.
                command('zload /Kernel/SerialDev/Message.ZC',
                        'zc: loaded /Kernel/SerialDev/Message.ZC')
            if args.probe_module == '/Kernel/KeyDev.ZC':
                command('zload /Kernel/SerialDev/Message.ZC',
                        'zc: loaded /Kernel/SerialDev/Message.ZC')
                command('zload /Kernel/Job.ZC', 'zc: loaded /Kernel/Job.ZC')
            if args.probe_module in ('/System/DolDoc/DocPutKey.ZC',
                                         '/System/DolDoc/DocPutS.ZC',
                                         '/System/DolDoc/DocDblBuf.ZC',
                                         '/System/DolDoc/DocTerm.ZC',
                                         '/System/DolDoc/DocInit.ZC',
                                         '/System/DolDoc/DocPlain.ZC',
                                         '/System/DolDoc/DocRecalcLib.ZC',
                                         '/System/DolDoc/DocHighlight.ZC',
                                         '/System/DolDoc/DocNew.ZC',
                                         '/System/DolDoc/DocBin.ZC',
                                         '/System/DolDoc/DocFile.ZC',
                                         '/System/DolDoc/DocForm.ZC',
                                         '/System/DolDoc/DocEd.ZC',
                                         '/System/DolDoc/DocGet.ZC'):
                command('zload /Kernel/SerialDev/Message.ZC',
                        'zc: loaded /Kernel/SerialDev/Message.ZC')
                command('zload /Kernel/Job.ZC', 'zc: loaded /Kernel/Job.ZC')
                command('zload /Kernel/KeyDev.ZC', 'zc: loaded /Kernel/KeyDev.ZC')
            if args.probe_module in ('/System/DolDoc/DocNew.ZC',
                                         '/System/DolDoc/DocPutKey.ZC',
                                         '/System/DolDoc/DocPutS.ZC',
                                         '/System/DolDoc/DocDblBuf.ZC',
                                         '/System/DolDoc/DocTerm.ZC',
                                         '/System/DolDoc/DocInit.ZC',
                                         '/System/DolDoc/DocPlain.ZC',
                                         '/System/DolDoc/DocRecalcLib.ZC',
                                         '/System/DolDoc/DocHighlight.ZC',
                                         '/System/DolDoc/DocFile.ZC',
                                         '/System/DolDoc/DocForm.ZC',
                                         '/System/DolDoc/DocEd.ZC',
                                         '/System/DolDoc/DocGet.ZC'):
                command('zload /System/DolDoc/DocBin.ZC',
                        'zc: loaded /System/DolDoc/DocBin.ZC')
            if args.probe_module in ('/System/DolDoc/DocPutKey.ZC',
                                         '/System/DolDoc/DocPutS.ZC',
                                         '/System/DolDoc/DocDblBuf.ZC',
                                         '/System/DolDoc/DocTerm.ZC',
                                         '/System/DolDoc/DocInit.ZC',
                                         '/System/DolDoc/DocPlain.ZC',
                                         '/System/DolDoc/DocRecalcLib.ZC',
                                         '/System/DolDoc/DocHighlight.ZC',
                                         '/System/DolDoc/DocFile.ZC',
                                         '/System/DolDoc/DocForm.ZC',
                                         '/System/DolDoc/DocEd.ZC',
                                         '/System/DolDoc/DocGet.ZC'):
                command('zload /System/DolDoc/DocNew.ZC',
                        'zc: loaded /System/DolDoc/DocNew.ZC')
            if args.probe_module in ('/System/DolDoc/DocPutS.ZC',
                                         '/System/DolDoc/DocDblBuf.ZC',
                                         '/System/DolDoc/DocTerm.ZC',
                                         '/System/DolDoc/DocInit.ZC',
                                         '/System/DolDoc/DocPlain.ZC',
                                         '/System/DolDoc/DocRecalcLib.ZC',
                                         '/System/DolDoc/DocHighlight.ZC',
                                         '/System/DolDoc/DocFile.ZC',
                                         '/System/DolDoc/DocForm.ZC',
                                         '/System/DolDoc/DocEd.ZC',
                                         '/System/DolDoc/DocGet.ZC'):
                command('zload /System/DolDoc/DocPutKey.ZC',
                        'zc: loaded /System/DolDoc/DocPutKey.ZC')
            if args.probe_module in ('/System/DolDoc/DocDblBuf.ZC',
                                         '/System/DolDoc/DocTerm.ZC',
                                         '/System/DolDoc/DocInit.ZC',
                                         '/System/DolDoc/DocPlain.ZC',
                                         '/System/DolDoc/DocRecalcLib.ZC',
                                         '/System/DolDoc/DocHighlight.ZC',
                                         '/System/DolDoc/DocFile.ZC',
                                         '/System/DolDoc/DocForm.ZC',
                                         '/System/DolDoc/DocEd.ZC',
                                         '/System/DolDoc/DocGet.ZC'):
                command('zload /System/DolDoc/DocPutS.ZC',
                        'zc: loaded /System/DolDoc/DocPutS.ZC')
            if args.probe_module in ('/System/DolDoc/DocTerm.ZC',
                                         '/System/DolDoc/DocInit.ZC',
                                         '/System/DolDoc/DocPlain.ZC',
                                         '/System/DolDoc/DocRecalcLib.ZC',
                                         '/System/DolDoc/DocHighlight.ZC',
                                         '/System/DolDoc/DocFile.ZC',
                                         '/System/DolDoc/DocForm.ZC',
                                         '/System/DolDoc/DocEd.ZC',
                                         '/System/DolDoc/DocGet.ZC'):
                # DocTermNew creates the shell's File menu, so MenuFile must
                # be defined before compiling DocTerm (the graphics startup
                # path already loads Menu.ZC before Window Manager modules).
                command('zload /System/DolDoc/DocDblBuf.ZC',
                        'zc: loaded /System/DolDoc/DocDblBuf.ZC')
            if args.probe_module in ('/System/DolDoc/DocInit.ZC',
                                         '/System/DolDoc/DocPlain.ZC',
                                         '/System/DolDoc/DocRecalcLib.ZC',
                                         '/System/DolDoc/DocHighlight.ZC',
                                         '/System/DolDoc/DocFile.ZC',
                                         '/System/DolDoc/DocForm.ZC',
                                         '/System/DolDoc/DocEd.ZC',
                                         '/System/DolDoc/DocGet.ZC'):
                command('zload /System/DolDoc/DocTerm.ZC',
                        'zc: loaded /System/DolDoc/DocTerm.ZC')
                command('zcall BootstrapDefinesLoad',
                        'zc => 0x0000000000000000')
            if args.probe_module in ('/System/DolDoc/DocPlain.ZC',
                                         '/System/DolDoc/DocRecalcLib.ZC',
                                         '/System/DolDoc/DocHighlight.ZC',
                                         '/System/DolDoc/DocFile.ZC',
                                         '/System/DolDoc/DocForm.ZC',
                                         '/System/DolDoc/DocEd.ZC',
                                         '/System/DolDoc/DocGet.ZC'):
                command('zload /System/DolDoc/DocInit.ZC',
                        'zc: loaded /System/DolDoc/DocInit.ZC')
            if args.probe_module in ('/System/DolDoc/DocRecalcLib.ZC',
                                     '/System/DolDoc/DocHighlight.ZC',
                                     '/System/DolDoc/DocFile.ZC',
                                         '/System/DolDoc/DocForm.ZC',
                                         '/System/DolDoc/DocEd.ZC',
                                         '/System/DolDoc/DocGet.ZC'):
                command('zload /System/DolDoc/DocPlain.ZC',
                        'zc: loaded /System/DolDoc/DocPlain.ZC')
            if args.probe_module == '/System/DolDoc/DocHighlight.ZC':
                command('zload /System/DolDoc/DocRecalcLib.ZC',
                        'zc: loaded /System/DolDoc/DocRecalcLib.ZC')
            if args.probe_module in ('/System/DolDoc/DocFile.ZC',
                                     '/System/DolDoc/DocForm.ZC',
                                     '/System/DolDoc/DocEd.ZC',
                                     '/System/DolDoc/DocGet.ZC'):
                command('zload /Tests/WinMgrProbeDeps.ZC',
                        'zc: loaded /Tests/WinMgrProbeDeps.ZC')
                command('zload /Tests/ODEsUpdateStub.ZC',
                        'zc: loaded /Tests/ODEsUpdateStub.ZC')
                command('zcall ODEsUpdateStubCheck',
                        'zc => 0x000000000000002a')
                command('zcall BootstrapGrContextFlagsInit',
                        'zc => 0x000000000000002a')
                command('zload /System/DolDoc/DocRecalcLib.ZC',
                        'zc: loaded /System/DolDoc/DocRecalcLib.ZC')
                command('zload /System/DolDoc/DocHighlight.ZC',
                        'zc: loaded /System/DolDoc/DocHighlight.ZC')
                command('zload /System/DolDoc/DocRecalc.ZC',
                        'zc: loaded /System/DolDoc/DocRecalc.ZC')
                if args.probe_module in ('/System/DolDoc/DocForm.ZC',
                                         '/System/DolDoc/DocEd.ZC',
                                         '/System/DolDoc/DocGet.ZC'):
                    command('zload /System/DolDoc/DocRun.ZC',
                            'zc: loaded /System/DolDoc/DocRun.ZC')
                if args.probe_module == '/System/DolDoc/DocEd.ZC':
                    command('zload /System/DolDoc/DocForm.ZC',
                            'zc: loaded /System/DolDoc/DocForm.ZC')
                if args.probe_module == '/System/DolDoc/DocGet.ZC':
                    command('zload /System/DolDoc/DocGet.ZC',
                            'zc: loaded /System/DolDoc/DocGet.ZC')
                if args.probe_module == '/System/DolDoc/DocEd.ZC':
                    command('zload /System/DolDoc/DocGet.ZC',
                            'zc: loaded /System/DolDoc/DocGet.ZC')
                    command('zload /System/DolDoc/DocChar.ZC',
                            'zc: loaded /System/DolDoc/DocChar.ZC')
                    command('zload /System/DolDoc/DocFind.ZC',
                            'zc: loaded /System/DolDoc/DocFind.ZC')
                    command('zload /System/DolDoc/DocLink.ZC',
                            'zc: loaded /System/DolDoc/DocLink.ZC')
                    command('zload /Tests/DocEdPopup.ZC',
                            'zc: loaded /Tests/DocEdPopup.ZC')
                    command('zcall DocEdPopupChecks',
                            'zc => 0x000000000000002a')
            if args.probe_module_override:
                override_data = args.probe_module_override.read_bytes()
                # The guest's persistent source volume only contains the
                # standard App directory. zrecv does not create parent dirs.
                override_result = upload_source('App/ModuleOverride.ZC', override_data)
                if 'zrecv: saved App/ModuleOverride.ZC' not in override_result:
                    raise RuntimeError('module override transfer failed:\n'+override_result)
                probe_path = 'disk:App/ModuleOverride.ZC'
            else:
                probe_path = args.probe_module
            guest.stdin.write(('zload '+probe_path+'\r').encode())
            guest.stdin.flush()
            result = until(b'\n> ')
            print('MODULE PROBE '+probe_path+'\n'+result, flush=True)
            if f'zc: loaded {probe_path}' not in result:
                raise RuntimeError(f'module probe failed to load: {probe_path}\n'+result)
            if args.probe_module == '/Tests/CallNOnStack.ZC':
                command('zcall CallNOnStackChecks',
                        'zc => 0x000000000000002a')
            if args.probe_module == '/Kernel/KTask.ZC':
                command('zload /Tests/KTaskCallStackGrow.ZC',
                        'zc: loaded /Tests/KTaskCallStackGrow.ZC')
                command('zload /Tests/KTaskTaskInit.ZC',
                        'zc: loaded /Tests/KTaskTaskInit.ZC')
                command('zcall KTaskTaskInitChecks',
                        'zc => 0x000000000000002a')
                command('zcall KTaskCallStackGrowChecks',
                        'zc => 0x000000000000002a')
                command('zcall KTaskCallBindingChecks',
                        'zc => 0x000000000000002a')
                command('zcall KTaskNamedHookChecks',
                        'zc => 0x000000000000002a')
                command('zcall KTaskSpawnBridgeChecks',
                        'zc => 0x000000000000002a')
                command('zcall KTaskKillBridgeChecks',
                        'zc => 0x000000000000002a')
            if args.probe_module == '/System/Math/MathODE.ZC':
                command('zload /Tests/D3Math.ZC',
                        'zc: loaded /Tests/D3Math.ZC')
                for check in ('D3EquCheck', 'D3NormCheck',
                              'D3ArithmeticCheck', 'D3OperationsCheck',
                              'F64AbsCheck', 'MathODECreateCheck',
                              'MathODERK5Check',
                              'MathODEUpdateLoopCheck'):
                    command(f'zcall {check}',
                            'zc => 0x0000000000000001')
            if args.probe_module == '/System/Win.ZC':
                if args.probe_grscreen:
                    command('zload /Tests/GrAsmColor4.ZC',
                            'zc: loaded /Tests/GrAsmColor4.ZC')
                    command('zcall GrAsmColor4Checks',
                            'zc => 0x000000000000002a')
                command('zcall WinExtTableCheck',
                        'zc => 0x0000000000000001')
                if args.probe_winmgr:
                    if args.probe_live_winmgr_ode:
                        command('zload /System/Math/MathODE.ZC',
                                'zc: loaded /System/Math/MathODE.ZC', timeout=30)
                        command('zload /Tests/D3Math.ZC',
                                'zc: loaded /Tests/D3Math.ZC')
                        for check in ('D3EquCheck', 'D3NormCheck',
                                      'D3ArithmeticCheck', 'D3OperationsCheck',
                                      'F64AbsCheck', 'MathODECreateCheck',
                                      'MathODERK5Check',
                                      'MathODEUpdateLoopCheck'):
                            command(f'zcall {check}',
                                    'zc => 0x0000000000000001')
                    deps = ('WinMgrProbeDepsMathODE' if args.probe_live_winmgr_ode
                            else 'WinMgrProbeDeps')
                    command(f'zload /Tests/{deps}.ZC',
                            f'zc: loaded /Tests/{deps}.ZC')
                    command('zload /System/Menu.ZC',
                            'zc: loaded /System/Menu.ZC', timeout=15)
                    command('zload /System/WinMgr.ZC',
                            'zc: loaded /System/WinMgr.ZC', timeout=15)
                    command('zload /System/Gr/WinMgrBoot.ZC',
                            'zc: loaded /System/Gr/WinMgrBoot.ZC', timeout=15)
                    if args.probe_grscreen:
                        if not args.probe_live_winmgr_ode:
                            command('zload /Tests/ODEsUpdateStub.ZC',
                                    'zc: loaded /Tests/ODEsUpdateStub.ZC')
                            command('zcall ODEsUpdateStubCheck',
                                    'zc => 0x000000000000002a')
                        guest.stdin.write(b'zload /System/Gr/GrScreen.ZC\r')
                        guest.stdin.flush()
                        result = until(b'\n> ', timeout=30)
                        if 'zc: loaded /System/Gr/GrScreen.ZC' not in result:
                            raise RuntimeError('ARM64-translated GrScreen did not compile:\n'+result)
                        print('PASS ARM64-translated GrScreen compiles', flush=True)
                        command('zload /Tests/GrScreenTextBG.ZC',
                                'zc: loaded /Tests/GrScreenTextBG.ZC')
                        command('zcall GrScreenTextBGChecks',
                                'zc => 0x000000000000002a')
                        command('zload /Tests/GrRopForeground.ZC',
                                'zc: loaded /Tests/GrRopForeground.ZC')
                        command('zcall GrRopForegroundChecks',
                                'zc => 0x000000000000002a')
                        command('zcall GrUpdateTextFGChecks',
                                'zc => 0x000000000000002a')
                        command('zcall GrUpdateScreen32Checks',
                                'zc => 0x000000000000002a')
                        command('zload /Tests/GrZoomScreen.ZC',
                                'zc: loaded /Tests/GrZoomScreen.ZC')
                        command('zcall GrZoomScreenChecks',
                                'zc => 0x000000000000002a')
                        command('zcall GrUpdateScreen32LimineChecks',
                                'zc => 0x000000000000002a')
                        command('zload /Tests/GrScreenFinalUpdate.ZC',
                                'zc: loaded /Tests/GrScreenFinalUpdate.ZC')
                        command('zcall GrScreenZoomTransformChecks',
                                'zc => 0x000000000000002a')
                        command('zcall BootstrapCtrlWheelZoomChecks',
                                'zc => 0x000000000000002a')
                        command('zcall BootstrapGrContextInitChecks',
                                'zc => 0x000000000000002a')
                        command('zcall BootstrapGrCircleBoundsChecks',
                                'zc => 0x000000000000002a')
                        command('zcall BootstrapGrPenTablesChecks',
                                'zc => 0x000000000000002a')
                        command('zcall GrScreenFinalUpdateChecks',
                                'zc => 0x000000000000002a')
                        command('zload /System/Ctrls/ViewAngles.ZC',
                                'zc: loaded /System/Ctrls/ViewAngles.ZC')
                        command('zload /Tests/ViewAngles.ZC',
                                'zc: loaded /Tests/ViewAngles.ZC')
                        command('zcall ViewAnglesControlChecks',
                                'zc => 0x000000000000002a')
                        if args.probe_live_winmgr:
                            command('zload /Tests/WinMgrStart.ZC',
                                    'zc: loaded /Tests/WinMgrStart.ZC')
                            command('zcall WinMgrTimingStateChecks',
                                    'zc => 0x000000000000002a')
                            command('zcall WinMgrKeyboardStateChecks',
                                    'zc => 0x000000000000002a')
                            command('zcall WinMgrCalcIdleChildChecks',
                                    'zc => 0x000000000000002a')
                            command('zcall WinMgrStartChecks',
                                    'zc => 0x000000000000002a', timeout=30)
                            if args.probe_live_window_control:
                                command('zload /Tests/WindowDragLive.ZC',
                                        'zc: loaded /Tests/WindowDragLive.ZC')
                                command('zcall WindowDragLiveManagerChecks',
                                        'zc => 0x000000000000002a', timeout=30)
                            if not args.probe_live_winmgr_ode:
                                command('zcall WinMgrMousePipelineChecks',
                                        'zc => 0x000000000000002a', timeout=30)
                            if not args.probe_live_winmgr_ode:
                                command('zcall WinMgrSourceDragChecks',
                                        'zc => 0x000000000000002a', timeout=30)
                            if args.probe_live_winmgr_ode:
                                command('zcall MathODEUpdateFramePrepare',
                                        'zc => 0x0000000000000001')
                                command('zcall MathODEUpdateFrameCheck',
                                        'zc => 0x0000000000000001', timeout=10)
                                command('winmgrstop',
                                        'winmgr: stopped; bootstrap framebuffer pump resumed')
                                command('zcall MathODEUpdateFrameCleanup',
                                        'zc => 0x0000000000000001')
                            if args.probe_live_winmgr_wake:
                                # Optional deeper probe; the regular acceptance
                                # gate verifies startup/first refresh only.
                                deadline = time.monotonic() + .5
                                while time.monotonic() < deadline:
                                    if select.select([guest.stdout], [], [], .05)[0]:
                                        data = os.read(guest.stdout.fileno(), 65536)
                                        if not data:
                                            raise RuntimeError('guest exited after WinMgr start')
                                        buf.extend(data); log.write(data); log.flush()
                                        fault_at = buf.find(b'sync: ESR=')
                                        if fault_at >= 0 and b'\n' in buf[fault_at:]:
                                            raise RuntimeError('WinMgr fault after timer wake\n' +
                                                               buf[-2000:].decode(errors='replace'))
                        if args.probe_fulltext:
                            if not args.probe_fulltext_visible:
                                command('fbquiet',
                                        'fbconsole: quiet (serial output remains enabled)')
                            command('zload /Tests/GrScreenTextFrame.ZC',
                                    'zc: loaded /Tests/GrScreenTextFrame.ZC')
                            command('zcall GrScreenTextFrameChecks',
                                    'zc => 0x000000000000002a', timeout=60)
            if args.probe_module == '/Kernel/SerialDev/Message.ZC':
                if 'zc: loaded /Kernel/SerialDev/Message.ZC' not in result:
                    raise RuntimeError('unchanged Message.ZC did not load')
                command('zload /Tests/TaskStringInput.ZC',
                        'zc: loaded /Tests/TaskStringInput.ZC')
                command_after_marker_key('zcall TaskOriginalKeyGetLive',
                                         'key get ready', b'q',
                                         'zc => 0x000000000000002a')
                # UTM Terminal sends CRLF for Return; the LF must not be
                # delivered to the original KeyGet as a second key press.
                command_after_marker_key('zcall TaskOriginalKeyGetLive',
                                         'key get ready', b'q',
                                         'zc => 0x000000000000002a',
                                         terminator=b'\r\n')
                command_after_marker_key('zcall TaskOriginalCharGetLive',
                                         'char get ready', b'r',
                                         'zc => 0x000000000000002a')
                command_after_marker_key('zcall TaskStringInputLive',
                                         'line input ready', b'ab\x08c\r\n',
                                         'zc => 0x000000000000002a')
                command_after_marker_key('zcall TaskStringDefaultLive',
                                         'default input ready', b'\r\n',
                                         'zc => 0x000000000000002a')
                command_after_marker_key('zcall TaskStringFixedLive',
                                         'fixed input ready', b'xy\r\n',
                                         'zc => 0x000000000000002a')
                command_after_marker_key('zcall TaskStringTruncateLive',
                                         'truncate input ready', b'abcd\r\n',
                                         'zc => 0x000000000000002a')
            if args.probe_module == '/Kernel/Job.ZC':
                if 'zc: loaded /Kernel/Job.ZC' not in result:
                    raise RuntimeError('unchanged Job.ZC did not load')
                command('zload /Tests/TaskOriginalJob.ZC',
                        'zc: loaded /Tests/TaskOriginalJob.ZC')
                command('zcall TaskOriginalJobChecks',
                        'zc => 0x000000000000002a')
            if args.probe_module == '/Kernel/KeyDev.ZC':
                if 'zc: loaded /Kernel/KeyDev.ZC' not in result:
                    raise RuntimeError('unchanged KeyDev.ZC did not load')
                command('zload /Tests/TaskOriginalKeyDev.ZC',
                        'zc: loaded /Tests/TaskOriginalKeyDev.ZC')
                command('zcall TaskOriginalKeyDevChecks',
                        'zc => 0x000000000000002a')
            if args.probe_module == '/System/DolDoc/DocBin.ZC':
                if 'zc: loaded /System/DolDoc/DocBin.ZC' not in result:
                    raise RuntimeError('unchanged DocBin.ZC did not load')
            if args.probe_module == '/System/DolDoc/DocNew.ZC':
                if 'zc: loaded /System/DolDoc/DocNew.ZC' not in result:
                    raise RuntimeError('unchanged DocNew.ZC did not load')
            if args.probe_module == '/System/DolDoc/DocPutKey.ZC':
                if 'zc: loaded /System/DolDoc/DocPutKey.ZC' not in result:
                    raise RuntimeError('unchanged DocPutKey.ZC did not load')
            if args.probe_module == '/System/DolDoc/DocPutS.ZC':
                if 'zc: loaded /System/DolDoc/DocPutS.ZC' not in result:
                    raise RuntimeError('unchanged DocPutS.ZC did not load')
                command('zload /Tests/TaskOriginalDocPutKey.ZC',
                        'zc: loaded /Tests/TaskOriginalDocPutKey.ZC')
                command('zcall TaskOriginalDocPutKeyChecks',
                        'zc => 0x000000000000002a')
            if args.probe_module == '/System/DolDoc/DocDblBuf.ZC':
                if 'zc: loaded /System/DolDoc/DocDblBuf.ZC' not in result:
                    raise RuntimeError('unchanged DocDblBuf.ZC did not load')
                command('zload /Tests/TaskOriginalDocPutKey.ZC',
                        'zc: loaded /Tests/TaskOriginalDocPutKey.ZC')
                command('zcall TaskOriginalDocPutKeyChecks',
                        'zc => 0x000000000000002a')
            if args.probe_module == '/System/DolDoc/DocTerm.ZC':
                if 'zc: loaded /System/DolDoc/DocTerm.ZC' not in result:
                    raise RuntimeError('unchanged DocTerm.ZC did not load')
                command('zload /Tests/TaskOriginalDocTerm.ZC',
                        'zc: loaded /Tests/TaskOriginalDocTerm.ZC')
                command('zcall TaskOriginalDocTermChecks',
                        'zc => 0x000000000000002a')
            if args.probe_module == '/System/DolDoc/DocInit.ZC':
                if 'zc: loaded /System/DolDoc/DocInit.ZC' not in result:
                    raise RuntimeError('unchanged DocInit.ZC did not load')
                command('zload /Tests/TaskOriginalDocInit.ZC',
                        'zc: loaded /Tests/TaskOriginalDocInit.ZC')
                command('zcall TaskOriginalDocInitChecks',
                        'zc => 0x000000000000002a')
            if args.probe_module in ('/System/DolDoc/DocPlain.ZC',
                                     '/System/DolDoc/DocFile.ZC'):
                expected_loaded_module = args.probe_module
                if f'zc: loaded {expected_loaded_module}' not in result:
                    raise RuntimeError(
                        f'unchanged {expected_loaded_module} did not load')
                command('zload /Tests/TaskOriginalDocPlain.ZC',
                        'zc: loaded /Tests/TaskOriginalDocPlain.ZC')
                command('zcall TaskOriginalDocPlainChecks',
                        'zc => 0x000000000000002a')
                command('zload /Tests/TaskDocData.ZC',
                        'zc: loaded /Tests/TaskDocData.ZC')
                command('zcall TaskDocDataChecks',
                        'zc => 0x000000000000002a')
                command('zcall TaskDocPlainSaveChecks',
                        'zc => 0x000000000000002a')
                if args.probe_module == '/System/DolDoc/DocFile.ZC':
                    command('zcall TaskDocFormattedSaveChecks',
                            'zc => 0x000000000000002a')
                command('zcall TaskFileMaskChecks',
                        'zc => 0x000000000000002a')
                command('zcall TaskSourceFileReadChecks',
                        'zc => 0x000000000000002a')
                if args.pci:
                    command('zcall TaskDiskFileReadChecks',
                            'zc => 0x000000000000002a')
                if args.pci:
                    command('zcall TaskFileApiRoundTripChecks',
                            'zc => 0x000000000000002a')
                    command('zcall TaskFileApiPersistChecks',
                            'zc => 0x000000000000002a')
                command('zcall TaskDocReadChecks',
                        'zc => 0x000000000000002a')
                if (not args.pci and
                        args.probe_module == '/System/DolDoc/DocFile.ZC'):
                    command('rscatalog', 'rscatalog ok')
                    command('rsrename UseAdd.ZC FilesFindRedSeaOnly.ZC',
                            'rsrename ok')
                    command('zcall TaskFilesFindChecks',
                            'zc => 0x000000000000002a')
                if args.pci:
                    command('zcall TaskDocWriteChecks',
                            'zc => 0x000000000000002a')
                    command('zcall TaskDocAbsoluteSourceWriteChecks',
                            'zc => 0x000000000000002a')
                    command('zcall TaskFileFindDirectoryFilterChecks',
                            'zc => 0x000000000000002a')
                    command('zcall TaskFilesFindChecks',
                            'zc => 0x000000000000002a')
                    command('zput Tests/CallNOnStack.ZC I64 NativeOverlayLoaded(){return 42;}',
                            'zput: saved Tests/CallNOnStack.ZC')
                    command('zput Tests/FilesFindOverlayOnly.ZC I64 OverlayOnly(){return 42;}',
                            'zput: saved Tests/FilesFindOverlayOnly.ZC')
                    command('zput Tests/FilesFindOverlayDir/Nested/Only.ZC I64 NestedOnly(){return 42;}',
                            'zput: saved Tests/FilesFindOverlayDir/Nested/Only.ZC')
                    command('zload disk:Tests/CallNOnStack.ZC',
                            'zc: loaded disk:Tests/CallNOnStack.ZC')
                    command('zcall NativeOverlayLoaded',
                            'zc => 0x000000000000002a')
                    command('zcall TaskFilesFindNativeOverlayChecks',
                            'zc => 0x000000000000002a')
                    command('zrm Tests/FilesFindOverlayDir/Nested/Only.ZC',
                            'zrm: deleted Tests/FilesFindOverlayDir/Nested/Only.ZC')
                    command('zcall TaskFilesFindNativeDirTombstoneChecks',
                            'zc => 0x000000000000002a')
                    command('zrm Tests/FilesFindOverlayOnly.ZC',
                            'zrm: deleted Tests/FilesFindOverlayOnly.ZC')
                    command('zrm Tests/CallNOnStack.ZC',
                            'zrm: deleted Tests/CallNOnStack.ZC')
                    command('zcall TaskFileFindTombstoneChecks',
                            'zc => 0x000000000000002a')
            if args.probe_module == '/System/DolDoc/DocRecalcLib.ZC':
                if 'zc: loaded /System/DolDoc/DocRecalcLib.ZC' not in result:
                    raise RuntimeError('unchanged DocRecalcLib.ZC did not load')
                command('zload /Tests/TaskOriginalDocRecalcLib.ZC',
                        'zc: loaded /Tests/TaskOriginalDocRecalcLib.ZC')
                command('zcall TaskOriginalDocRecalcLibChecks',
                        'zc => 0x000000000000002a')
            if args.probe_module == '/System/DolDoc/DocHighlight.ZC':
                if 'zc: loaded /System/DolDoc/DocHighlight.ZC' not in result:
                    raise RuntimeError('unchanged DocHighlight.ZC did not load')
                command('zload /Tests/TaskOriginalDocHighlight.ZC',
                        'zc: loaded /Tests/TaskOriginalDocHighlight.ZC')
                command('zcall TaskOriginalDocHighlightChecks',
                        'zc => 0x000000000000002a')
            if args.probe_module == '/System/DolDoc/DocRecalc.ZC':
                if 'zc: loaded /System/DolDoc/DocRecalc.ZC' not in result:
                    raise RuntimeError('unchanged DocRecalc.ZC did not load')
                command('zload /Tests/WinMgrProbeDeps.ZC',
                        'zc: loaded /Tests/WinMgrProbeDeps.ZC')
                command('zload /Tests/ODEsUpdateStub.ZC',
                        'zc: loaded /Tests/ODEsUpdateStub.ZC')
                command('zcall ODEsUpdateStubCheck',
                        'zc => 0x000000000000002a')
                command('zcall BootstrapGrContextFlagsInit',
                        'zc => 0x000000000000002a')
                command('zcall BootstrapWinMgrStart',
                        'zc => 0x000000000000002a', timeout=30)
                command('zload /Tests/WinProgressBars.ZC',
                        'zc: loaded /Tests/WinProgressBars.ZC')
                command('zload /Tests/TaskOriginalDocRecalc.ZC',
                        'zc: loaded /Tests/TaskOriginalDocRecalc.ZC')
                command('zcall TaskOriginalDocRecalcChecks',
                        'zc => 0x000000000000002a')
                command('zload /Tests/WindowDragLive.ZC',
                        'zc: loaded /Tests/WindowDragLive.ZC')
                command('zcall WindowDragLiveButtonChecks',
                        'zc => 0x0000000000000000')
                result = command('zcall WindowDragLiveStart',
                                 'zc => 0x000000000000002a')
                if 'unresolved external call' in result:
                    raise RuntimeError('WindowDragLive redraw hit an unresolved external call:\n'+result)
                command('zcall WindowDragLiveRenderChecks',
                        'zc => 0x000000000000002a')
                command('zcall WindowDragLiveStop',
                        'zc => 0x0000000000000000')
            raise SystemExit(0)
        if args.probe_task_jobs:
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Kernel/KMathB.ZC', 'zc: loaded /Kernel/KMathB.ZC')
            command('zload /Kernel/KernelC.HH', 'zc: loaded /Kernel/KernelC.HH')
            command('zload /System/TaskBridge.ZC', 'zc: loaded /System/TaskBridge.ZC')
            command('zload /Tests/KernelStackRegisterChecks.ZC',
                    'zc: loaded /Tests/KernelStackRegisterChecks.ZC')
            command('zcall KernelStackRegisterChecks',
                    'zc => 0x000000000000002a')
            command('zload /Tests/TaskJobs.ZC', 'zc: loaded /Tests/TaskJobs.ZC')
            command('zcall TaskSourceCoopChecks', 'zc => 0x000000000000002a')
            command('zcall TaskNestedSourceChecks', 'zc => 0x000000000000002a')
            command('zcall TaskSourceExitChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskJobQueue.ZC', 'zc: loaded /Tests/TaskJobQueue.ZC')
            command('zcall TaskJobQueueChecks', 'zc => 0x000000000000002a')
            command('zcall TaskJobQueueCoopChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskPopup.ZC', 'zc: loaded /Tests/TaskPopup.ZC')
            command('zcall PopupBridgeChecks', 'zc => 0x000000000000002a')
            raise SystemExit(0)
        if args.probe_upstream:
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Tests/KernelStackRegisterChecks.ZC',
                    'zc: loaded /Tests/KernelStackRegisterChecks.ZC')
            command('zcall KernelStackRegisterChecks',
                    'zc => 0x000000000000002a')
            command('zload /Tests/HeaderCopy.ZC', 'zc: loaded /Tests/HeaderCopy.ZC')
            command('zcall HeaderCopyChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/KernelBitOps.ZC', 'zc: loaded /Tests/KernelBitOps.ZC')
            command('zcall KernelBitOpsChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskIRQFlags.ZC', 'zc: loaded /Tests/TaskIRQFlags.ZC')
            command('zcall TaskIRQFlagsChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskKeyboardPoll.ZC', 'zc: loaded /Tests/TaskKeyboardPoll.ZC')
            command_after_marker_key('zcall TaskKeyboardPollLive', 'keyboard poll ready', b'p',
                                     'zc => 0x0000000000000070')
            checked_task_print()
            command('zload /Tests/TaskQueues.ZC', 'zc: loaded /Tests/TaskQueues.ZC')
            command('zcall TaskQueueChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskExceptions.ZC', 'zc: loaded /Tests/TaskExceptions.ZC')
            command('zcall TaskExceptionChecks', 'zc => 0x000000000000002a')
            command('zcall TaskExceptionBareChecks', 'zc => 0x000000000000002a')
            command('zcall TaskExceptionCatchNestedChecks', 'zc => 0x000000000000002a')
            command('zload /Kernel/KMathB.ZC', 'zc: loaded /Kernel/KMathB.ZC')
            command('zload /Tests/TaskMath.ZC', 'zc: loaded /Tests/TaskMath.ZC')
            command('zcall TaskMathChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskSwitch.ZC', 'zc: loaded /Tests/TaskSwitch.ZC')
            command('zcall TaskSwitchChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/SpawnBridge.ZC', 'zc: loaded /Tests/SpawnBridge.ZC')
            command('zcall SpawnBridgeChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskLifecycle.ZC', 'zc: loaded /Tests/TaskLifecycle.ZC')
            command('zcall TaskLifecycleChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskEnd.ZC', 'zc: loaded /Tests/TaskEnd.ZC')
            command('zcall TaskEndChecks', 'zc => 0x000000000000002a')
            command('zcall TaskKillCallbackChecks', 'zc => 0x000000000000002a')
            command('zcall TaskJobEndCallbackChecks', 'zc => 0x000000000000002a')
            command('zcall TaskFamilyEndChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskWait.ZC', 'zc: loaded /Tests/TaskWait.ZC')
            command('zcall TaskWaitChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskIdle.ZC', 'zc: loaded /Tests/TaskIdle.ZC')
            command('zcall TaskIdleChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskMessages.ZC', 'zc: loaded /Tests/TaskMessages.ZC')
            command('zcall TaskMessageChecks', 'zc => 0x000000000000002a')
            command('zcall TaskMessageMoveCoalesceChecks', 'zc => 0x000000000000002a')
            command('zcall PopupMessageChecks', 'zc => 0x000000000000002a')
            command('zcall TaskKeyDescriptionChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskPopup.ZC', 'zc: loaded /Tests/TaskPopup.ZC')
            command('zcall PopupBridgeChecks', 'zc => 0x000000000000002a')
            command_after_marker_key('zcall PopupLiveKey', 'popup key ready', b'p',
                                     'zc => 0x0000000000000070', delay=.3,
                                     virtio=True)
            command('zload /Tests/TaskFocusRing.ZC', 'zc: loaded /Tests/TaskFocusRing.ZC')
            command('zcall TaskFocusRingChecks', 'zc => 0x000000000000002a')
            command_after_marker_key('zcall TaskFocusRingLive', 'focus ring ready', b'r',
                                     'zc => 0x0000000000000072', delay=.3,
                                     virtio=True)
            command('zload /Tests/TaskChildRing.ZC', 'zc: loaded /Tests/TaskChildRing.ZC')
            command('zcall TaskChildRingChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskJobs.ZC', 'zc: loaded /Tests/TaskJobs.ZC')
            command('zcall TaskExeChecks', 'zc => 0x000000000000002a')
            command('zcall TaskSourceCoopChecks', 'zc => 0x000000000000002a')
            command('zcall TaskNestedSourceChecks', 'zc => 0x000000000000002a')
            command('zcall TaskSourceExitChecks', 'zc => 0x000000000000002a')
            command('zcall TaskExeLifecycleChecks', 'zc => 0x000000000000002a')
            command('zcall TaskExeFocusChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskTimer.ZC', 'zc: loaded /Tests/TaskTimer.ZC')
            command('zcall TaskTimerChecks', 'zc => 0x000000000000002a')
            command('zcall TaskTimerRunChecks', 'zc => 0x000000000000002a')
            command('zcall TaskTimerSuspendChecks', 'zc => 0x000000000000002a')
            command('zcall TaskTimerJobChecks', 'zc => 0x000000000000002a')
            command('zcall TaskSleepUntilChecks', 'zc => 0x000000000000002a')
            command('zcall TaskTimerWaitChecks', 'zc => 0x000000000000002a')
            command('zcall TaskDocLockChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskText.ZC', 'zc: loaded /Tests/TaskText.ZC')
            command('zcall TaskAnswerChecks', 'zc => 0x000000000000002a')
            command('zcall TaskTextChecks', 'zc => 0x000000000000002a')
            command('zcall TaskTextPriorityChecks', 'zc => 0x000000000000002a')
            checked_task_recycling()
            command('zcall TaskCompilerScopeChecks', 'zc => 0x000000000000002a')
            checked_shell_answers()
            command('zload /Tests/TaskMessageJobs.ZC', 'zc: loaded /Tests/TaskMessageJobs.ZC')
            command('zcall ShellFloatAnswerChecks', 'zc => 0x000000000000002a')
            command('zcall TaskMessageJobsChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskKeys.ZC', 'zc: loaded /Tests/TaskKeys.ZC')
            command('zcall TaskKeyChecks', 'zc => 0x000000000000002a')
            checked_put_key()
            command_with_key('zcall TaskLiveKey', b'k', 'zc => 0x000000000000006b')
            command_with_key('zcall TaskLiveMessage', b'm', 'zc => 0x000000000000006d')
            command_with_key('zcall TaskLiveScan', b'n', 'zc => 0x000000000000006e')
            command('zload /Tests/TaskChars.ZC', 'zc: loaded /Tests/TaskChars.ZC')
            command('zcall TaskCharQueueChecks', 'zc => 0x000000000000002a')
            command_after_marker_key('zcall TaskCharLive', 'char scan ready', b'c',
                                     'zc => 0x0000000000000063', delay=.3)
            command('zcall TaskFocusKillChecks', 'zc => 0x000000000000002a')
            checked_background_tasks()
            command_after_marker_key('zcall TaskLiveFocus', 'focus ready', b'f',
                                     'zc => 0x0000000000000066',
                                     virtio=True)
            command('zload /Tests/TaskSpawnQueue.ZC', 'zc: loaded /Tests/TaskSpawnQueue.ZC')
            command('zcall TaskSpawnQueueChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskJobQueue.ZC', 'zc: loaded /Tests/TaskJobQueue.ZC')
            command('zcall TaskJobQueueChecks', 'zc => 0x000000000000002a')
            command('zcall TaskJobQueueCoopChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskExePrint.ZC', 'zc: loaded /Tests/TaskExePrint.ZC')
            command('zcall TaskExePrintChecks', 'zc => 0x000000000000002a')
            command('zcall TaskExceptionFiberChecks', 'zc => 0x000000000000002a')
            command('zcall TaskIdleDeadlockChecks', 'task wait cannot progress')
            command('zreset', 'native compiler ready')
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Tests/TaskWait.ZC', 'zc: loaded /Tests/TaskWait.ZC')
            command('zcall TaskWaitDeadlockChecks', 'task wait cannot progress')
            command('zreset', 'native compiler ready')
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Tests/TaskMessages.ZC', 'zc: loaded /Tests/TaskMessages.ZC')
            command('zcall TaskMessageDeadlockChecks', 'message wait cannot progress')
            command('zreset', 'native compiler ready')
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Tests/TaskText.ZC', 'zc: loaded /Tests/TaskText.ZC')
            command('zcall TaskTextKillParentChecks', 'kill current or ancestor during job dispatch is unsupported')
            command('zreset', 'native compiler ready')
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Tests/TaskText.ZC', 'zc: loaded /Tests/TaskText.ZC')
            command('zcall TaskCompilerScopeChecks', 'zc => 0x000000000000002a')
            command('zc ChildOnly();', 'compile failed')
            command('zreset', 'native compiler ready')
            raise SystemExit(0)
        command('zcheck', 'zc: upstream QuickSort + persistent modules OK')
        if args.pci:
            command('zverify Kernel/KernelA.HH', 'zverify: OK bytes=113880')
            command('zreset', 'native compiler ready')
            command('zload disk:Kernel/QuickSort.ZC', 'zc: loaded disk:Kernel/QuickSort.ZC')
            command('zload disk:Tests/SortCaller.ZC', 'zc: loaded disk:Tests/SortCaller.ZC')
            command('zcall SortChecks', 'zc => 0x000000000000003f')
            command('zcheck', 'zc: upstream QuickSort + persistent modules OK')
        if not args.quick:
            command('zload /Tests/LocalArray.ZC', 'zc: loaded /Tests/LocalArray.ZC')
            command('zcall LocalArrayChecks', 'zc => 0x000000000000002a')
            command('zc F64 FPResult(){return 1.5;}', 'zc: executed')
            command('zcall FPResult', 'zcall requires an integer return type')
            command('zcall NextRun', 'zc => 0x0000000000000003')
            command('zc U8 *surv=MAlloc(32);', 'zc: executed')
            command('hc return 7;', '0x0000000000000007')
            command('zc I64 SizeSurv(){return MSize(surv);}', 'zc: executed')
            command('zcall SizeSurv', 'zc => 0x0000000000000020')
            command('zc Free(surv);', 'zc: executed')
            command('zcall SortChecks', 'zc => 0x000000000000003f')
            command('zload /missing.ZC', 'source not found')
            command('zcall SortChecks', 'zc => 0x000000000000003f')
            before = command('zstatus', 'failed=0')
            for _ in range(8):
                command('zcall SortChecks', 'zc => 0x000000000000003f')
            after = command('zstatus', 'failed=0')
            heap = r'data=([0-9/]+) code=([0-9/]+) allocations=([0-9]+)'
            if re.search(heap, before).groups() != re.search(heap, after).groups():
                raise RuntimeError('sorting leaked memory')
            command('zc I64 Broken( {', 'compile failed')
            command('zstatus', 'failed=0')
            command('zcheck', 'zc: upstream QuickSort + persistent modules OK')
            # Keep this beyond the bridge's current 96 MiB data-arena bound.
            command('zc MAlloc(100663297);', 'allocation size out of range')
            command('zstatus', 'failed=1')
            command('zreset', 'native compiler ready')
            command('zc try {} catch {}', 'Guest try/catch requires the task bridge')
            command('zcheck', 'zc: upstream QuickSort + persistent modules OK')
            command('zload /Tests/Directives.ZC', 'zc: loaded /Tests/Directives.ZC')
            command('zcall DirectiveChecks', 'zc => 0x000000000000002d')
            command('zload /Tests/MathReturns.ZC', 'zc: loaded /Tests/MathReturns.ZC')
            command('zcall MathReturnChecks', 'zc => 0x000000000000000f')
            command('zload /Tests/ConditionalClass.ZC', 'zc: loaded /Tests/ConditionalClass.ZC')
            command('zcall ConditionalClassChecks', 'zc => 0x0000000000000004')
            command('zload /Tests/ClassLayout.ZC', 'zc: loaded /Tests/ClassLayout.ZC')
            command('zcall LayoutCursorChecks', 'zc => 0x0000000000000050')
            command('zload /Tests/DefineCategory.ZC', 'zc: loaded /Tests/DefineCategory.ZC')
            command('zcall DefineCategoryChecks', 'zc => 0x0000000000000007')
            command('zload /Tests/MathTable.ZC', 'zc: loaded /Tests/MathTable.ZC')
            command('zcall MathTableChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/ClassLayoutReject.ZC', 'Class layout offset out of range')
            command('zstatus', 'failed=0')
            command('zreset', 'native compiler ready')
            command('zload /Tests/NamedExternDecl.ZC', 'zc: loaded /Tests/NamedExternDecl.ZC')
            command('zload /Tests/NamedImportDecl.ZC', 'zc: loaded /Tests/NamedImportDecl.ZC')
            command('zload /Tests/NamedExternCallReject.ZC', "AArch64 external 'NamedExtern' (REMOTE_PROC) is not bound")
            command('zstatus', 'failed=0')
            command('zreset', 'native compiler ready')
            command('zload /Tests/NamedExternDecl.ZC', 'zc: loaded /Tests/NamedExternDecl.ZC')
            command('zload /Tests/NamedExternDataReject.ZC', "AArch64 external data 'named_extern_data' is not bound")
            command('zstatus', 'failed=0')
            command('zreset', 'native compiler ready')
            command('zload /Tests/BindingReuse.ZC', 'zc: loaded /Tests/BindingReuse.ZC')
            command('zcall ReboundCopy', 'zc => 0x000000000000002a')
            command('zload /Tests/BindingMismatch.ZC', "AArch64 intrinsic 'SwapI64' (IC_SWAP_I64) is not implemented")
            command('zstatus', 'failed=0')
            command('zreset', 'native compiler ready')
            command('zload /Tests/IntrinsicDecl.ZC', 'zc: loaded /Tests/IntrinsicDecl.ZC')
            command('zload /Tests/IntrinsicReject.ZC', "AArch64 intrinsic 'Bsf' (IC_BSF) is not implemented")
            command('zstatus', 'failed=0')
            command('zreset', 'native compiler ready')
            command('zload /Tests/AssertFailure.ZC', '#assert failed')
            command('zstatus', 'failed=0')
            command('zreset', 'native compiler ready')
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Tests/TaskAssertSwitch.ZC',
                    'zc: loaded /Tests/TaskAssertSwitch.ZC')
            command('zcall TaskAssertSwitchChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskAssertFun.ZC',
                    'zc: loaded /Tests/TaskAssertFun.ZC')
            command('zcall TaskAssertFunChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskUnboundedSwitch.ZC',
                    'zc: loaded /Tests/TaskUnboundedSwitch.ZC')
            command('zcall TaskUnboundedSwitchChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/HeaderCopy.ZC', 'zc: loaded /Tests/HeaderCopy.ZC')
            command('zcall HeaderCopyChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/KernelBitOps.ZC', 'zc: loaded /Tests/KernelBitOps.ZC')
            command('zcall KernelBitOpsChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskIRQFlags.ZC', 'zc: loaded /Tests/TaskIRQFlags.ZC')
            command('zcall TaskIRQFlagsChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskKeyboardPoll.ZC', 'zc: loaded /Tests/TaskKeyboardPoll.ZC')
            command_after_marker_key('zcall TaskKeyboardPollLive', 'keyboard poll ready', b'p',
                                     'zc => 0x0000000000000070')
            checked_task_print()
            command('zload /Tests/TaskQueues.ZC', 'zc: loaded /Tests/TaskQueues.ZC')
            command('zcall TaskQueueChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskExceptions.ZC', 'zc: loaded /Tests/TaskExceptions.ZC')
            command('zcall TaskExceptionChecks', 'zc => 0x000000000000002a')
            command('zcall TaskExceptionBareChecks', 'zc => 0x000000000000002a')
            command('zcall TaskExceptionCatchNestedChecks', 'zc => 0x000000000000002a')
            command('zload /Kernel/KMathB.ZC', 'zc: loaded /Kernel/KMathB.ZC')
            command('zload /Tests/TaskMath.ZC', 'zc: loaded /Tests/TaskMath.ZC')
            command('zcall TaskMathChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskSwitch.ZC', 'zc: loaded /Tests/TaskSwitch.ZC')
            command('zcall TaskSwitchChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/SpawnBridge.ZC', 'zc: loaded /Tests/SpawnBridge.ZC')
            command('zcall SpawnBridgeChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskLifecycle.ZC', 'zc: loaded /Tests/TaskLifecycle.ZC')
            command('zcall TaskLifecycleChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskEnd.ZC', 'zc: loaded /Tests/TaskEnd.ZC')
            command('zcall TaskEndChecks', 'zc => 0x000000000000002a')
            command('zcall TaskKillCallbackChecks', 'zc => 0x000000000000002a')
            command('zcall TaskJobEndCallbackChecks', 'zc => 0x000000000000002a')
            command('zcall TaskFamilyEndChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskWait.ZC', 'zc: loaded /Tests/TaskWait.ZC')
            command('zcall TaskWaitChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskIdle.ZC', 'zc: loaded /Tests/TaskIdle.ZC')
            command('zcall TaskIdleChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskMessages.ZC', 'zc: loaded /Tests/TaskMessages.ZC')
            command('zcall TaskMessageChecks', 'zc => 0x000000000000002a')
            command('zcall TaskMessageMoveCoalesceChecks', 'zc => 0x000000000000002a')
            command('zcall PopupMessageChecks', 'zc => 0x000000000000002a')
            command('zcall TaskKeyDescriptionChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskPopup.ZC', 'zc: loaded /Tests/TaskPopup.ZC')
            command('zcall PopupBridgeChecks', 'zc => 0x000000000000002a')
            command_after_marker_key('zcall PopupLiveKey', 'popup key ready', b'p',
                                     'zc => 0x0000000000000070', delay=.3,
                                     virtio=True)
            command('zload /Tests/TaskFocusRing.ZC', 'zc: loaded /Tests/TaskFocusRing.ZC')
            command('zcall TaskFocusRingChecks', 'zc => 0x000000000000002a')
            command_after_marker_key('zcall TaskFocusRingLive', 'focus ring ready', b'r',
                                     'zc => 0x0000000000000072', delay=.3,
                                     virtio=True)
            command('zload /Tests/TaskChildRing.ZC', 'zc: loaded /Tests/TaskChildRing.ZC')
            command('zcall TaskChildRingChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskJobs.ZC', 'zc: loaded /Tests/TaskJobs.ZC')
            command('zcall TaskExeChecks', 'zc => 0x000000000000002a')
            command('zcall TaskSourceCoopChecks', 'zc => 0x000000000000002a')
            command('zcall TaskNestedSourceChecks', 'zc => 0x000000000000002a')
            command('zcall TaskSourceExitChecks', 'zc => 0x000000000000002a')
            command('zcall TaskExeLifecycleChecks', 'zc => 0x000000000000002a')
            command('zcall TaskExeFocusChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskTimer.ZC', 'zc: loaded /Tests/TaskTimer.ZC')
            command('zcall TaskTimerChecks', 'zc => 0x000000000000002a')
            command('zcall TaskTimerRunChecks', 'zc => 0x000000000000002a')
            command('zcall TaskTimerSuspendChecks', 'zc => 0x000000000000002a')
            command('zcall TaskTimerJobChecks', 'zc => 0x000000000000002a')
            command('zcall TaskSleepUntilChecks', 'zc => 0x000000000000002a')
            command('zcall TaskTimerWaitChecks', 'zc => 0x000000000000002a')
            command('zcall TaskDocLockChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskText.ZC', 'zc: loaded /Tests/TaskText.ZC')
            command('zcall TaskAnswerChecks', 'zc => 0x000000000000002a')
            command('zcall TaskTextChecks', 'zc => 0x000000000000002a')
            command('zcall TaskTextPriorityChecks', 'zc => 0x000000000000002a')
            checked_task_recycling()
            command('zcall TaskCompilerScopeChecks', 'zc => 0x000000000000002a')
            checked_shell_answers()
            command('zload /Tests/TaskMessageJobs.ZC', 'zc: loaded /Tests/TaskMessageJobs.ZC')
            command('zcall ShellFloatAnswerChecks', 'zc => 0x000000000000002a')
            command('zcall TaskMessageJobsChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskKeys.ZC', 'zc: loaded /Tests/TaskKeys.ZC')
            command('zcall TaskKeyChecks', 'zc => 0x000000000000002a')
            checked_put_key()
            command_with_key('zcall TaskLiveKey', b'k', 'zc => 0x000000000000006b')
            command_with_key('zcall TaskLiveMessage', b'm', 'zc => 0x000000000000006d')
            command_with_key('zcall TaskLiveScan', b'n', 'zc => 0x000000000000006e')
            command('zload /Tests/TaskChars.ZC', 'zc: loaded /Tests/TaskChars.ZC')
            command('zcall TaskCharQueueChecks', 'zc => 0x000000000000002a')
            command_after_marker_key('zcall TaskCharLive', 'char scan ready', b'c',
                                     'zc => 0x0000000000000063', delay=.3)
            command('zcall TaskFocusKillChecks', 'zc => 0x000000000000002a')
            checked_background_tasks()
            command_after_marker_key('zcall TaskLiveFocus', 'focus ready', b'f',
                                     'zc => 0x0000000000000066',
                                     virtio=True)
            command('zload /Tests/TaskSpawnQueue.ZC', 'zc: loaded /Tests/TaskSpawnQueue.ZC')
            command('zcall TaskSpawnQueueChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskJobQueue.ZC', 'zc: loaded /Tests/TaskJobQueue.ZC')
            command('zcall TaskJobQueueChecks', 'zc => 0x000000000000002a')
            command('zcall TaskJobQueueCoopChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskExePrint.ZC', 'zc: loaded /Tests/TaskExePrint.ZC')
            command('zcall TaskExePrintChecks', 'zc => 0x000000000000002a')
            command('zcall TaskExceptionFiberChecks', 'zc => 0x000000000000002a')
            command('zcall TaskIdleDeadlockChecks', 'task wait cannot progress')
            command('zreset', 'native compiler ready')
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Tests/TaskWait.ZC', 'zc: loaded /Tests/TaskWait.ZC')
            command('zcall TaskWaitDeadlockChecks', 'task wait cannot progress')
            command('zreset', 'native compiler ready')
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Tests/TaskMessages.ZC', 'zc: loaded /Tests/TaskMessages.ZC')
            command('zcall TaskMessageDeadlockChecks', 'message wait cannot progress')
            command('zreset', 'native compiler ready')
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Tests/TaskText.ZC', 'zc: loaded /Tests/TaskText.ZC')
            command('zcall TaskTextKillParentChecks', 'kill current or ancestor during job dispatch is unsupported')
            command('zreset', 'native compiler ready')
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Tests/TaskText.ZC', 'zc: loaded /Tests/TaskText.ZC')
            command('zcall TaskCompilerScopeChecks', 'zc => 0x000000000000002a')
            command('zc ChildOnly();', 'compile failed')
            command('zreset', 'native compiler ready')
        # Use only the disposable scratch volume. Source bytes must survive a
        # complete guest restart, not merely remain in the compiler session.
        command('rspersist', 'rspersist ok')
        if args.pci:
            # The task-isolation checks above deliberately zreset the source
            # compiler and its loaded desktop modules. Keep this final editor
            # pass focused on native-file persistence; --probe-source-editor
            # boots a fresh desktop and owns the graphical scanout assertions.
            editor_command('zedit App/Editor.ZC', 'zedit: new App/Editor.ZC')
            editor_command('a I64 Edited(){return 17;}', 'zedit: added line 1')
            editor_command('n // cursor insertion', 'zedit: inserted line 1')
            editor_command('j', 'zedit: cursor line 2')
            editor_command('e I64 Edited(){return 42;}', 'zedit: edited line 2')
            editor_command('p 1 2', '2: I64 Edited(){return 42;}')
            editor_command('G', 'zedit: cursor line 2')
            editor_command('[', 'zedit: cursor line 1')
            editor_command(']', 'zedit: cursor line 2')
            editor_command('g', 'zedit: cursor line 1')
            editor_command('q', 'zedit: unsaved changes; use wq or q!')
            editor_command('wq', 'zedit: saved App/Editor.ZC', finish=True)
            command('zload disk:App/Editor.ZC', 'zc: loaded disk:App/Editor.ZC')
            command('zcall Edited', 'zc => 0x000000000000002a')
            editor_command('zedit App/Editor.ZC', 'zedit: opened App/Editor.ZC')
            editor_command('i 1 // temporary first line', 'zedit: inserted line 1')
            editor_command('p 1 3', '3: I64 Edited(){return 42;}')
            editor_command('d 1', 'zedit: deleted line 1')
            editor_command('w', 'zedit: saved App/Editor.ZC')
            editor_command('j', 'zedit: cursor line 2')
            editor_command('k', 'zedit: cursor line 1')
            editor_command('j', 'zedit: cursor line 2')
            editor_command('x', 'zedit: deleted selected line')
            editor_command('q!', 'zedit: discarded changes', finish=True)
            editor_command('zedit App/Editor.ZC', 'zedit: opened App/Editor.ZC')
            editor_command('wq', 'zedit: unchanged', finish=True)
            command('zput Lib/Shared.HH I64 FromHeader(){return 40;}', 'zput: saved Lib/Shared.HH')
            command('zput App/Native.ZC #include "../Lib/Shared.HH"\\nI64 DiskValue(){return FromHeader()+2;}', 'zput: saved App/Native.ZC')
            command('zput App/Remove.ZC I64 Removed(){return 17;}', 'zput: saved App/Remove.ZC')
            command('zls', 'App/Remove.ZC bytes=25')
            command('zrm App/Remove.ZC', 'zrm: deleted App/Remove.ZC')
            command('zrm App/Remove.ZC', 'zrm: missing source')
            command('zverify App/Remove.ZC', 'zverify: missing or damaged source')
            listed = command('zls', 'zls: n=')
            if 'App/Remove.ZC bytes=' in listed:
                raise RuntimeError('deleted path remained in zls:\n'+listed)
            command('zput App/Remove.ZC I64 Removed(){return 42;}', 'zput: saved App/Remove.ZC')
            command('zload disk:App/Remove.ZC', 'zc: loaded disk:App/Remove.ZC')
            command('zcall Removed', 'zc => 0x000000000000002a')
            native = 'disk:App/Native.ZC'
            transferred = (b'I64 TransferValue(){return 42;}\n' +
                           b'// serial source transfer keeps lines and bytes intact\n' * 28)
            result = upload_source('App/Transfer.ZC', transferred)
            if 'zrecv: saved App/Transfer.ZC' not in result:
                raise RuntimeError('serial source transfer failed:\n'+result)
            print(f'PASS zrecv App/Transfer.ZC ({len(transferred)} bytes)', flush=True)
            listed = command('zls', 'zls: n=')
            if 'App/Transfer.ZC bytes=' not in listed:
                raise RuntimeError('native source listing omitted transferred file:\n'+listed)
            wrong = transferred.replace(b'return 42', b'return 13')
            result = upload_source('App/Transfer.ZC', wrong, zlib.crc32(wrong) ^ 1)
            if 'zrecv: checksum mismatch; old file intact' not in result:
                raise RuntimeError('bad transfer was not rejected:\n'+result)
            print('PASS zrecv checksum rejects replacement', flush=True)
            command('zreset', 'native compiler ready')
            command('zload disk:App/Transfer.ZC', 'zc: loaded disk:App/Transfer.ZC')
            command('zcall TransferValue', 'zc => 0x000000000000002a')
        else:
            command('zedit Native.ZC', 'zedit: native source partition unavailable')
            command('zput Shared.HH I64 FromHeader(){return 40;}', 'zput: saved Shared.HH')
            command('zput Native.ZC #include "Shared.HH"\\nI64 DiskValue(){return FromHeader()+2;}', 'zput: saved Native.ZC')
            native = 'disk:Native.ZC'
        command('zreset', 'native compiler ready')
        command('zload '+native, 'zc: loaded '+native)
        command('zcall DiskValue', 'zc => 0x000000000000002a')
        if args.pci:
            command('zput Hi.ZC I64 Hidden(){return 12;}', 'zput: saved Hi.ZC')
            command('zrm Hi.ZC', 'zrm: deleted Hi.ZC')
            command('zload disk:Hi.ZC', 'zc: source not found: disk:Hi.ZC')
            before_gc = command('zls', 'zls: n=')
            old_use = int(re.search(r'sectors=(\d+)/', before_gc).group(1))
            command('zgc', 'zgc: OK sectors=')
            command('zvol', 'bank=B generation=1')
            after_gc = command('zls', 'zls: n=')
            new_use = int(re.search(r'sectors=(\d+)/', after_gc).group(1))
            if new_use >= old_use:
                raise RuntimeError(f'guest compaction did not reclaim space: {old_use}->{new_use}')
            command('zverify Kernel/KernelA.HH', 'zverify: OK bytes=113880')
            command('zload disk:Hi.ZC', 'zc: source not found: disk:Hi.ZC')
        else:
            command('zgc', 'zgc: native source partition unavailable')
        guest.terminate()
        try: guest.wait(timeout=5)
        except subprocess.TimeoutExpired: guest.kill(); guest.wait()
        guest = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, bufsize=0)
        buf.clear()
        # Reboot recompiles the same desktop source set as the initial boot.
        boot = until(b'\n> ', timeout=180)
        if 'zc: native compiler ready' not in boot:
            raise RuntimeError('compiler unavailable after reboot')
        if args.pci:
            command('zvol', 'bank=B generation=1')
            command('zverify Kernel/KernelA.HH', 'zverify: OK bytes=113880')
            command('zload disk:Hi.ZC', 'zc: source not found: disk:Hi.ZC')
            command('zput App/BankB.ZC I64 BankB(){return 42;}', 'zput: saved App/BankB.ZC')
            command('zreset', 'native compiler ready')
            command('zload disk:App/BankB.ZC', 'zc: loaded disk:App/BankB.ZC')
            command('zcall BankB', 'zc => 0x000000000000002a')
            command('zgc', 'bank=A')
            command('zvol', 'bank=A generation=2')
            guest.terminate()
            try: guest.wait(timeout=5)
            except subprocess.TimeoutExpired: guest.kill(); guest.wait()
            compact = subprocess.check_output(
                ['python3', str(ROOT/'scripts/source-volume.py'), 'compact',
                 str(tmp/'disk.img')], text=True)
            print('PASS offline '+compact.strip(), flush=True)
            guest = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT, bufsize=0)
            buf.clear()
            boot = until(b'\n> ', timeout=180)
            if 'zc: native compiler ready' not in boot:
                raise RuntimeError('compiler unavailable after second reboot')
            command('zvol', 'bank=A generation=2')
            command('rspersist', 'rspersist ok')
            command('zload disk:Hi.ZC', 'zc: source not found: disk:Hi.ZC')
            command('zreset', 'native compiler ready')
            command('zls', 'App/Remove.ZC bytes=25')
            command('zload disk:App/BankB.ZC', 'zc: loaded disk:App/BankB.ZC')
            command('zcall BankB', 'zc => 0x000000000000002a')
        command('zload '+native, 'zc: loaded '+native)
        command('zcall DiskValue', 'zc => 0x000000000000002a')
        if args.pci:
            command('zreset', 'native compiler ready')
            command('zload disk:App/Transfer.ZC', 'zc: loaded disk:App/Transfer.ZC')
            command('zcall TransferValue', 'zc => 0x000000000000002a')
            command('zreset', 'native compiler ready')
            command('zload disk:App/Editor.ZC', 'zc: loaded disk:App/Editor.ZC')
            command('zcall Edited', 'zc => 0x000000000000002a')
        print(f'PASS compatibility ({"PCI disk" if args.pci else "ISO"}); log: {log_path}', flush=True)
    finally:
        guest.terminate()
        try: guest.wait(timeout=5)
        except subprocess.TimeoutExpired: guest.kill(); guest.wait()
