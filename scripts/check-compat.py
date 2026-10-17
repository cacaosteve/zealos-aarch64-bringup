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
p.add_argument('--probe-task-settings', action='store_true',
               help='check upstream application settings and task-local directories at normal desktop boot')
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
if args.probe_module_override and not args.probe_module:
    p.error('--probe-module-override requires --probe-module')
if args.probe_fulltext_visible:
    args.probe_fulltext = True
if args.probe_source and not args.pci:
    p.error('--probe-source requires --pci')
if args.probe_gui_terminal and (
        not args.pci or args.probe_module != '/System/DolDoc/DocRecalc.ZC'):
    p.error('--probe-gui-terminal requires --pci --probe-module /System/DolDoc/DocRecalc.ZC')
if args.probe_gui_keyboard and not args.pci:
    p.error('--probe-gui-keyboard requires --pci')
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
log_path = ROOT / 'build' / (f'check-usb-{usb_label}.log' if args.usb_input else
                             'check-live-window-control.log' if args.probe_live_window_control else
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
        cmd += ['-drive', f'if=none,format=raw,file={tmp}/disk.img,id=esp', '-device', 'virtio-blk-pci,drive=esp,bootindex=0']
    else:
        redsea = bytearray(65536)
        if args.probe_module == '/System/DolDoc/DocPlain.ZC':
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
        guest.stdin.write((line+'\r').encode()); guest.stdin.flush()
        result = until(b'\n> ', timeout=timeout)
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
    def qmp_send_keys(qcodes):
        qmp_command('send-key', {
            'keys': [{'type': 'qcode', 'data': qcode} for qcode in qcodes],
            'hold-time': 50})
    def graphical_path(name):
        screen_name = f'usb-{usb_label}-{name}' if args.usb_input else name
        return ROOT/'build'/f'gui-{screen_name}.ppm'
    def graphical_screen(name):
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
        rows = []
        # The source planar font starts one scanline below the text cell.
        for y in range(1, height-7, 8):
            row = ''
            for x in range(0, width-7, 8):
                bits = tuple(sum(0x80 >> col for col in range(8)
                                 if pixels[((y+r)*width+x+col)*3:
                                           ((y+r)*width+x+col)*3+3] == b'\xff\xff\xff')
                             for r in range(8))
                row += font.get(bits, font.get(tuple(v ^ 255 for v in bits), '?'))
            rows.append(row.rstrip())
        text = '\n'.join(rows)
        path.with_suffix('.txt').write_text(text+'\n')
        return width, height, pixels, text
    def require_graphical_text(name, expected):
        deadline = time.monotonic() + 5
        while True:
            screen = graphical_screen(name)
            if expected in screen[3]:
                break
            if time.monotonic() >= deadline:
                raise RuntimeError(f'graphical scanout is missing {expected!r}; '
                                   f'see {graphical_path(name)} and .txt\n'+screen[3])
            time.sleep(0.1)
        print(f'PASS visible graphical text: {expected!r}', flush=True)
        return screen
    def white_pixels(screen, x, y, width, height):
        stride, _, pixels, _ = screen
        return sum(pixels[((y+r)*stride+x+c)*3:((y+r)*stride+x+c)*3+3]
                   == b'\xff\xff\xff' for r in range(height) for c in range(width))
    def require_graphical_caret(screen, line):
        deadline = time.monotonic() + 3
        while True:
            for row, text in enumerate(screen[3].splitlines()):
                if line in text:
                    col = text.index(line) + len(line)
                    if white_pixels(screen, col*8, row*8, 8, 8) >= 8:
                        print('PASS visible graphical insertion caret', flush=True)
                        return
                    break
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
    def focus_graphical_terminal():
        location = command('zc Print("gui_click=%d,%d\\n", '
            '(bootstrap_gui_terminal_task->pix_left+bootstrap_gui_terminal_task->pix_right)/2, '
            '(bootstrap_gui_terminal_task->pix_top+bootstrap_gui_terminal_task->pix_bottom)/2);',
            'gui_click=')
        point = re.search(r'gui_click=(-?\d+),(-?\d+)', location)
        if not point:
            raise RuntimeError('could not read graphical terminal bounds:\n'+location)
        move_graphical_pointer(int(point.group(1)), int(point.group(2)))
        for down in (True, False):
            qmp_command('input-send-event', {'events': [
                {'type': 'btn', 'data': {'down': down, 'button': 'left'}}]})
            time.sleep(0.15)
        command('zstatus', 'keyboard_focus=Terminal')
    def qmp_send_text(text, delay=0.2):
        qcodes = {
            ' ': ('spc',), '/': ('slash',), '.': ('dot',),
            ';': ('semicolon',), ':': ('shift', 'semicolon'), '\n': ('ret',),
            '(': ('shift', '9'), ')': ('shift', '0'),
            '{': ('shift', 'bracket_left'), '}': ('shift', 'bracket_right'),
        }
        for char in text:
            keys = qcodes.get(char)
            if keys is None:
                keys = ('shift', char.lower()) if char.isupper() else (char,)
            qmp_send_keys(keys)
            # Keep longer lines below the guest's cooperative input polling
            # rate; QMP acknowledges injection before the source task consumes
            # each event, and an early Return can otherwise overtake the tail.
            time.sleep(delay)
    def checked_put_key():
        command('zload /Tests/TaskPutKey.ZC', 'zc: loaded /Tests/TaskPutKey.ZC')
        result = command('zcall TaskPutKeyChecks', 'zc => 0x000000000000002a')
        if '<>' not in result or '~' in result:
            raise RuntimeError('PutKey echo/silent output mismatch: '+result)
        print('PASS PutKey text echo and silent flag', flush=True)
    def checked_task_print():
        command('zload /Tests/TaskPrint.ZC', 'zc: loaded /Tests/TaskPrint.ZC')
        result = command('zcall TaskPrintChecks', 'zc => 0x000000000000002a')
        if 'A text % BC\n tail\n' not in result.replace('\r\n', '\n'):
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
        boot = until(b'\n> ')
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
        print('PASS boot and legacy regression markers', flush=True)
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
        redraws = command('zcall BootstrapGuiTerminalRedrawCount', 'zc => 0x')
        redraw_count = re.search(r'zc => 0x([0-9a-fA-F]+)', redraws)
        if not redraw_count or int(redraw_count.group(1), 16) == 0:
            raise RuntimeError('boot compositor has not redrawn the terminal document:\n' + redraws)
        print(f'PASS boot terminal document redraws: {int(redraw_count.group(1), 16)}', flush=True)
        visible_text = command('zcall BootstrapGuiTerminalTextPixelCount', 'zc => 0x')
        text_pixels = re.search(r'zc => 0x([0-9a-fA-F]+)', visible_text)
        if not text_pixels or int(text_pixels.group(1), 16) < 100:
            raise RuntimeError('boot graphical terminal has no visible document text pixels:\n' + visible_text)
        print(f'PASS boot graphical terminal visible text pixels: {int(text_pixels.group(1), 16)}', flush=True)
        command('zload /Tests/TaskSettings.ZC', 'zc: loaded /Tests/TaskSettings.ZC')
        command('zcall TaskCurrentDirectoryChecks', 'zc => 0x000000000000002a')
        command('zcall TaskSettingsRestoreChecks', 'zc => 0x000000000000002a')
        command('zcall TaskSettingsDocumentChecks', 'zc => 0x000000000000002a')
        command('zstatus', 'failed=0')
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
        if args.probe_gui_keyboard:
            require_graphical_text('boot-terminal', 'ZealOS ARM64 graphical terminal')
            raw_pattern = r'usb_input:.* key_events=(\d+)' if args.usb_input else r'virtio_keyboard: ready=1 key_events=(\d+)'
            raw_before = re.search(raw_pattern, desktop)
            if not raw_before:
                raise RuntimeError('could not read low-level virtio keyboard event count:\n' + desktop)
            before = command('zcall BootstrapGuiTerminalKeyEventCount', 'zc => 0x')
            before_count = re.search(r'zc => 0x([0-9a-fA-F]+)', before)
            if not before_count:
                raise RuntimeError('could not read graphical-terminal key-event count:\n' + before)
            for key in ('h', 'e', 'l', 'p'):
                qmp_send_key(key)
            time.sleep(0.3)
            typing = require_graphical_text('typing-before-return', '> help')
            require_graphical_pointer()
            require_graphical_caret(typing, '> help')
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
                require_graphical_text('usb-fast-typing', '> '+fast_command)
                qmp_send_key('ret')
                require_graphical_text('usb-fast-submit', 'zc: executed')
                # Definitions belong to the graphical task's compiler scope.
                # Invoke it there rather than from the independent UART task.
                qmp_send_text('zcall UsbFastTyping')
                qmp_send_key('ret')
                require_graphical_text('usb-fast-run', 'zc => 0x000000000000002a')
                print('PASS native USB mixed-case/modifier typing at ten characters per second', flush=True)
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
            command('zc I64 GuiEditorEdCheck(){if(Ed("/System/DolDoc/DocEd.ZC",EDF_BAIL|EDf_BAIL))return 42;return 1;}',
                    'zc: executed')
            command('zcall GuiEditorEdCheck',
                    'zc => 0x000000000000002a')
            editor_command = 'edit /System/DolDoc/DocEd.ZC'
            qmp_send_text(editor_command)
            qmp_send_key('ret')
            # The terminal task enters upstream DocEd() and waits for keys.
            # Send navigation and Escape through the same virtio path, then
            # check those keys were consumed by the editor instead of the
            # terminal command loop and that the terminal accepts input again.
            time.sleep(0.5)
            command('zcall BootstrapGuiTerminalLastCommandIsEdit',
                    'zc => 0x0000000000000001')
            editor_open = command('zstatus', 'gui_editor_active=')
            if 'gui_editor_active=1' not in editor_open:
                raise RuntimeError('GUI editor returned before accepting input:\n' + editor_open)
            if 'keyboard_focus=Terminal' not in editor_open:
                raise RuntimeError('GUI editor lost the focused terminal task:\n' + editor_open)
            editor_scanout = re.search(r'scanout: nonblack_pixels=(\d+) of 480000', editor_open)
            if not editor_scanout or int(editor_scanout.group(1)) < 1000:
                raise RuntimeError('GUI editor did not paint visible document pixels:\n' + editor_open)
            count_while_editing = command(
                'zcall BootstrapGuiTerminalKeyEventCount', 'zc => 0x')
            count_while_editing_value = re.search(
                r'zc => 0x([0-9a-fA-F]+)', count_while_editing)
            if not count_while_editing_value:
                raise RuntimeError('could not read graphical-terminal key count while editor was open:\n' + count_while_editing)
            for key in ('left', 'right', 'esc'):
                qmp_send_key(key)
                time.sleep(0.1)
            time.sleep(0.4)
            command('zstatus', 'gui_editor_active=0')
            after_editor = command('zcall BootstrapGuiTerminalKeyEventCount', 'zc => 0x')
            after_editor_count = re.search(r'zc => 0x([0-9a-fA-F]+)', after_editor)
            if (not after_editor_count or
                    int(after_editor_count.group(1), 16) !=
                    int(count_while_editing_value.group(1), 16)):
                raise RuntimeError('GUI editor navigation/Escape did not stay in the editor task:\n' + after_editor)
            for key in ('z', 'l', 's', 'ret'):
                qmp_send_key(key)
            time.sleep(0.3)
            command('zcall BootstrapGuiTerminalLastCommandIsZls',
                    'zc => 0x0000000000000001')
            print('PASS QMP GUI editor opened the bundled DocEd source, consumed navigation/Escape keys, and returned to the graphical shell', flush=True)
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
            for key in ('z', 'l', 's', 'ret'):
                qmp_send_key(key)
            time.sleep(0.3)
            command('zcall BootstrapGuiTerminalLastCommandIsZls',
                    'zc => 0x0000000000000001')
            print('PASS QMP graphical window demo launch/stop restored the terminal and keyboard focus', flush=True)
            if args.probe_gui_workflow:
                editor_path = '/Tests/GuiBringup.ZC'
                source = 'I64 GuiBringup(){return 42;}'
                qmp_send_text('edit '+editor_path)
                qmp_send_key('ret')
                time.sleep(0.5)
                command('zstatus', 'gui_editor_active=1')
                qmp_send_text(source)
                require_graphical_text('editor-typed-source', source)
                qmp_send_key('esc')
                time.sleep(0.5)
                command('zstatus', 'gui_editor_active=0')
                qmp_send_text('zload disk:Tests/GuiBringup.ZC')
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
                qmp_send_text('edit '+editor_path)
                require_graphical_text('editor-reopen-command', '> edit '+editor_path)
                qmp_send_key('ret')
                require_graphical_text('editor-reopened-after-reboot', source)
                command('zstatus', 'gui_editor_active=1')
                qmp_send_keys(['shift', 'esc'])
                time.sleep(0.5)
                command('zstatus', 'gui_editor_active=0')
                cancelled = require_graphical_text(
                    'editor-cancelled-after-reboot',
                    'The serial shell remains available.')
                if 'edit: could not open document' in cancelled[3]:
                    raise RuntimeError('cancelling an open editor reported an open failure')
                print('PASS graphical edit/save/compile/run/reboot/reopen workflow', flush=True)
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
            raise SystemExit(0)
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
                command('zload /Tests/GrSpriteBridge.ZC',
                        'zc: loaded /Tests/GrSpriteBridge.ZC')
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
                # GrRect can take the native probabilistic-dither path;
                # its random helper is supplied by KMathB, which zreset
                # removed from the boot-loaded compiler session.
                command('zload /Kernel/KMathB.ZC',
                        'zc: loaded /Kernel/KMathB.ZC', timeout=30)
                command('zload /System/Gr/GrBitMap.ZC',
                        'zc: loaded /System/Gr/GrBitMap.ZC', timeout=30)
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
                command('zload /Tests/GrSpriteTransform.ZC',
                        'zc: loaded /Tests/GrSpriteTransform.ZC')
                if not args.probe_live_winmgr_ode:
                    command('zcall GrSpriteTransformChecks',
                            'zc => 0x000000000000002a')
                command('zload /Tests/GrDCLifecycle.ZC',
                        'zc: loaded /Tests/GrDCLifecycle.ZC')
                command('zcall GrDCLifecycleChecks',
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
                upload_source('Probe/ModuleOverride.ZC', override_data)
                probe_path = 'disk:Probe/ModuleOverride.ZC'
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
            command('zstatus', 'failed=1')
            command('zcheck', 'zc: upstream QuickSort + persistent modules OK')
            command('zc MAlloc(33554432);', 'allocation size out of range')
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
            command('zstatus', 'failed=1')
            command('zreset', 'native compiler ready')
            command('zload /Tests/NamedExternDecl.ZC', 'zc: loaded /Tests/NamedExternDecl.ZC')
            command('zload /Tests/NamedImportDecl.ZC', 'zc: loaded /Tests/NamedImportDecl.ZC')
            command('zload /Tests/NamedExternCallReject.ZC', "AArch64 external 'NamedExtern' (REMOTE_PROC) is not bound")
            command('zstatus', 'failed=1')
            command('zreset', 'native compiler ready')
            command('zload /Tests/NamedExternDecl.ZC', 'zc: loaded /Tests/NamedExternDecl.ZC')
            command('zload /Tests/NamedExternDataReject.ZC', "AArch64 external data 'named_extern_data' is not bound")
            command('zstatus', 'failed=1')
            command('zreset', 'native compiler ready')
            command('zload /Tests/BindingReuse.ZC', 'zc: loaded /Tests/BindingReuse.ZC')
            command('zcall ReboundCopy', 'zc => 0x000000000000002a')
            command('zload /Tests/BindingMismatch.ZC', "AArch64 intrinsic 'SwapI64' (IC_SWAP_I64) is not implemented")
            command('zstatus', 'failed=1')
            command('zreset', 'native compiler ready')
            command('zload /Tests/IntrinsicDecl.ZC', 'zc: loaded /Tests/IntrinsicDecl.ZC')
            command('zload /Tests/IntrinsicReject.ZC', "AArch64 intrinsic 'Bsf' (IC_BSF) is not implemented")
            command('zstatus', 'failed=1')
            command('zreset', 'native compiler ready')
            command('zload /Tests/AssertFailure.ZC', '#assert failed')
            command('zstatus', 'failed=1')
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
        boot = until(b'\n> ')
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
            boot = until(b'\n> ')
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
