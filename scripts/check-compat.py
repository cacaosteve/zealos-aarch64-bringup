#!/usr/bin/env python3
"""Exercise the native compiler inside a disposable QEMU/HVF guest."""
import argparse, os, pathlib, re, select, shlex, shutil, struct, subprocess, tempfile, time, zlib
ROOT = pathlib.Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--pci', action='store_true')
p.add_argument('--quick', action='store_true', help='only compiler integration gate')
p.add_argument('--probe-upstream', action='store_true', help='check current full-header/subsystem compiler results')
p.add_argument('--probe-module', metavar='ARCHIVE_PATH',
               help='load an unchanged upstream source after KernelA/B in a disposable guest')
p.add_argument('--probe-task-jobs', action='store_true', help='check CPU-0 callback jobs after loading pinned task headers')
p.add_argument('--probe-source', type=pathlib.Path, action='append',
               help='load a host source file from the disposable PCI source partition')
args = p.parse_args()
if args.probe_source and not args.pci:
    p.error('--probe-source requires --pci')
log_path = ROOT / 'build' / ('check-module-probe.log' if args.probe_module else
                             'check-upstream-probe.log' if args.probe_upstream else
                             'check-compat-pci.log' if args.pci else 'check-compat.log')
with tempfile.TemporaryDirectory(prefix='zeal-compat-') as tmp, log_path.open('wb') as log:
    tmp = pathlib.Path(tmp)
    fw = os.environ.get('FW_CODE', '/opt/homebrew/share/qemu/edk2-aarch64-code.fd')
    shutil.copyfile(os.environ.get('FW_VARS_IN', '/opt/homebrew/share/qemu/edk2-arm-vars.fd'), tmp/'vars.fd')
    platform_args = shlex.split(os.environ.get('QEMU_VIRT', '-machine virt,accel=hvf,gic-version=3 -cpu host -m 512 -device ramfb -device virtio-keyboard-device -device virtio-tablet-device'))
    cmd = [os.environ.get('QEMU', 'qemu-system-aarch64')] + platform_args + ['-drive', f'if=pflash,unit=0,format=raw,file={fw},readonly=on', '-drive', f'if=pflash,unit=1,format=raw,file={tmp}/vars.fd', '-serial', 'stdio', '-monitor', 'none', '-display', 'none']
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
        (tmp/'redsea.img').write_bytes(bytes(65536))
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
                if b'sync: ESR=' in buf and marker not in buf:
                    raise RuntimeError('guest synchronous fault\n'+buf[-2000:].decode(errors='replace'))
        end = buf.index(marker)+len(marker)
        result = bytes(buf[:end]); del buf[:end]
        return result.decode(errors='replace')
    def command(line, expected):
        guest.stdin.write((line+'\r').encode()); guest.stdin.flush()
        result = until(b'\n> ')
        if expected not in result: raise RuntimeError(f'{line}: missing {expected!r}\n{result}')
        print(f'PASS {line}: {expected}', flush=True)
        return result
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
        guest.stdin.write(b'g'); guest.stdin.flush()
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
                                 terminator=b'\r'):
        guest.stdin.write(line.encode()+terminator); guest.stdin.flush()
        before = until(marker.encode())
        if delay:
            time.sleep(delay)
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
        for marker in ('hc IR OK (front+host)', 'virtio-blk: rw OK', 'zc: native compiler ready'):
            if marker not in boot: raise RuntimeError(f'boot missing {marker}\n{boot[-4000:]}')
        if re.search(r'hc IR FAIL|(?:Upstream|Front) [^\r\n]* FAIL', boot):
            raise RuntimeError('legacy regressions')
        print('PASS boot and legacy regression markers', flush=True)
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
                guest.stdin.write(f'zload disk:Probe/Source{i}.ZC\r'.encode()); guest.stdin.flush()
                print('SOURCE PROBE '+str(source)+'\n'+until(b'\n> '), flush=True)
                command('zreset', 'native compiler ready')
            raise SystemExit(0)
        if args.probe_module:
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            guest.stdin.write(b'zload /Kernel/KernelB.HH\r')
            guest.stdin.flush()
            kb = until(b'\n> ')
            print('KERNELB LOAD\n'+kb, flush=True)
            if 'zc: loaded /Kernel/KernelB.HH' not in kb:
                raise RuntimeError('KernelB.HH did not load')
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
                                         '/System/DolDoc/DocNew.ZC',
                                         '/System/DolDoc/DocBin.ZC'):
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
                                         '/System/DolDoc/DocPlain.ZC'):
                command('zload /System/DolDoc/DocBin.ZC',
                        'zc: loaded /System/DolDoc/DocBin.ZC')
            if args.probe_module in ('/System/DolDoc/DocPutKey.ZC',
                                         '/System/DolDoc/DocPutS.ZC',
                                         '/System/DolDoc/DocDblBuf.ZC',
                                         '/System/DolDoc/DocTerm.ZC',
                                         '/System/DolDoc/DocInit.ZC',
                                         '/System/DolDoc/DocPlain.ZC'):
                command('zload /System/DolDoc/DocNew.ZC',
                        'zc: loaded /System/DolDoc/DocNew.ZC')
            if args.probe_module in ('/System/DolDoc/DocPutS.ZC',
                                         '/System/DolDoc/DocDblBuf.ZC',
                                         '/System/DolDoc/DocTerm.ZC',
                                         '/System/DolDoc/DocInit.ZC',
                                         '/System/DolDoc/DocPlain.ZC'):
                command('zload /System/DolDoc/DocPutKey.ZC',
                        'zc: loaded /System/DolDoc/DocPutKey.ZC')
            if args.probe_module in ('/System/DolDoc/DocDblBuf.ZC',
                                         '/System/DolDoc/DocTerm.ZC',
                                         '/System/DolDoc/DocInit.ZC',
                                         '/System/DolDoc/DocPlain.ZC'):
                command('zload /System/DolDoc/DocPutS.ZC',
                        'zc: loaded /System/DolDoc/DocPutS.ZC')
            if args.probe_module in ('/System/DolDoc/DocTerm.ZC',
                                         '/System/DolDoc/DocInit.ZC',
                                         '/System/DolDoc/DocPlain.ZC'):
                command('zload /System/DolDoc/DocDblBuf.ZC',
                        'zc: loaded /System/DolDoc/DocDblBuf.ZC')
            if args.probe_module in ('/System/DolDoc/DocInit.ZC',
                                         '/System/DolDoc/DocPlain.ZC'):
                command('zload /System/DolDoc/DocTerm.ZC',
                        'zc: loaded /System/DolDoc/DocTerm.ZC')
                command('zcall BootstrapDefinesLoad',
                        'zc => 0x0000000000000000')
            if args.probe_module == '/System/DolDoc/DocPlain.ZC':
                command('zload /System/DolDoc/DocInit.ZC',
                        'zc: loaded /System/DolDoc/DocInit.ZC')
            guest.stdin.write(('zload '+args.probe_module+'\r').encode())
            guest.stdin.flush()
            result = until(b'\n> ')
            print('MODULE PROBE '+args.probe_module+'\n'+result, flush=True)
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
            if args.probe_module == '/System/DolDoc/DocPlain.ZC':
                if 'zc: loaded /System/DolDoc/DocPlain.ZC' not in result:
                    raise RuntimeError('unchanged DocPlain.ZC did not load')
                command('zload /Tests/TaskOriginalDocPlain.ZC',
                        'zc: loaded /Tests/TaskOriginalDocPlain.ZC')
                command('zcall TaskOriginalDocPlainChecks',
                        'zc => 0x000000000000002a')
            raise SystemExit(0)
        if args.probe_task_jobs:
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
            command('zload /Tests/TaskJobs.ZC', 'zc: loaded /Tests/TaskJobs.ZC')
            command('zcall TaskSourceCoopChecks', 'zc => 0x000000000000002a')
            command('zcall TaskNestedSourceChecks', 'zc => 0x000000000000002a')
            command('zcall TaskSourceExitChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskJobQueue.ZC', 'zc: loaded /Tests/TaskJobQueue.ZC')
            command('zcall TaskJobQueueChecks', 'zc => 0x000000000000002a')
            command('zcall TaskJobQueueCoopChecks', 'zc => 0x000000000000002a')
            raise SystemExit(0)
        if args.probe_upstream:
            command('zload /Kernel/KernelA.HH', 'zc: loaded /Kernel/KernelA.HH')
            command('zload /Kernel/KernelB.HH', 'zc: loaded /Kernel/KernelB.HH')
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
            command('zcall PopupMessageChecks', 'zc => 0x000000000000002a')
            command('zcall TaskKeyDescriptionChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskPopup.ZC', 'zc: loaded /Tests/TaskPopup.ZC')
            command('zcall PopupBridgeChecks', 'zc => 0x000000000000002a')
            command_after_marker_key('zcall PopupLiveKey', 'popup key ready', b'p',
                                     'zc => 0x0000000000000070', delay=.3)
            command('zload /Tests/TaskFocusRing.ZC', 'zc: loaded /Tests/TaskFocusRing.ZC')
            command('zcall TaskFocusRingChecks', 'zc => 0x000000000000002a')
            command_after_marker_key('zcall TaskFocusRingLive', 'focus ring ready', b'r',
                                     'zc => 0x0000000000000072', delay=.3)
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
                                     'zc => 0x0000000000000066')
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
            command('zcall PopupMessageChecks', 'zc => 0x000000000000002a')
            command('zcall TaskKeyDescriptionChecks', 'zc => 0x000000000000002a')
            command('zload /Tests/TaskPopup.ZC', 'zc: loaded /Tests/TaskPopup.ZC')
            command('zcall PopupBridgeChecks', 'zc => 0x000000000000002a')
            command_after_marker_key('zcall PopupLiveKey', 'popup key ready', b'p',
                                     'zc => 0x0000000000000070', delay=.3)
            command('zload /Tests/TaskFocusRing.ZC', 'zc: loaded /Tests/TaskFocusRing.ZC')
            command('zcall TaskFocusRingChecks', 'zc => 0x000000000000002a')
            command_after_marker_key('zcall TaskFocusRingLive', 'focus ring ready', b'r',
                                     'zc => 0x0000000000000072', delay=.3)
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
                                     'zc => 0x0000000000000066')
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
            editor_command('c 1 I64 Edited(){return 42;}', 'zedit: changed line 1')
            editor_command('p 1 1', '1: I64 Edited(){return 42;}')
            editor_command('q', 'zedit: unsaved changes; use wq or q!')
            editor_command('wq', 'zedit: saved App/Editor.ZC', finish=True)
            command('zload disk:App/Editor.ZC', 'zc: loaded disk:App/Editor.ZC')
            command('zcall Edited', 'zc => 0x000000000000002a')
            editor_command('zedit App/Editor.ZC', 'zedit: opened App/Editor.ZC')
            editor_command('i 1 // temporary first line', 'zedit: inserted line 1')
            editor_command('p 1 2', '2: I64 Edited(){return 42;}')
            editor_command('d 1', 'zedit: deleted line 1')
            editor_command('w', 'zedit: saved App/Editor.ZC')
            editor_command('a // discarded change', 'zedit: added line 2')
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
