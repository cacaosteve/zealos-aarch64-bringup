#!/usr/bin/env python3
"""Create a disposable candidate disk with ToTheFront movement reporting."""
import argparse
import pathlib
import shutil
import subprocess
import tarfile
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--disk', required=True, type=pathlib.Path)
p.add_argument('--sources', required=True, type=pathlib.Path)
p.add_argument('--output', required=True, type=pathlib.Path)
p.add_argument('--direct-fire', action='store_true',
               help='also arrange adjacent opponents to exercise both direct-fire phases')
p.add_argument('--fix-visibility', action='store_true',
               help='apply the current-player visibility fix to an older source bundle')
a = p.parse_args()
if a.output.exists():
    p.error('output exists; choose a fresh disposable disk path')
with tarfile.open(a.sources) as bundle:
    source = bundle.extractfile('Apps/ToTheFront/AIs/Human.ZC').read().decode('utf-8')
if a.fix_visibility:
    old_visibility = '!Bt(&target->vis, 0)'
    if source.count(old_visibility) != 1:
        raise SystemExit('expected exactly one old direct-fire visibility check')
    source = source.replace(old_visibility, '!Bt(&target->vis[cur_player], 0)', 1)
old = '''U0 PlayerMove()
{
\tUnit *tmpu = NULL;
\tI64   message_code, arg1, arg2;

\tViewPlayerSet(cur_player);'''
new = '''U0 PlayerMove()
{
 Unit *tmpu = NULL, *tu;
 I64 message_code, arg1, arg2, ti, td, tr, tc, old_r, old_c;
 F64 tx, ty, ux, uy;
 Bool found = FALSE;
 ViewPlayerSet(cur_player);
 for (ti=0; ti<UNITS_NUM && !found; ti++) {
  tu=&units[cur_player][ti];
  if (tu->life>0 && tu->remaining_movement) {
   for (td=0; td<6 && !found; td++) {
    tr=tu->row; tc=tu->col; Toward(&tr,&tc,td);
    if (tr>=0 && tr<map_rows && tc>=0 && tc<map_cols && !UnitFind(tr,tc) &&
        HexMoveOneCost(tu,tr,tc,td)<=tu->remaining_movement) {
     RowCol2XY(&tx,&ty,tu->row,tu->col); RowCol2XY(&ux,&uy,tr,tc);
     if (tx+x0>=20 && tx+x0<620 && ty+y0>=30 && ty+y0<460 &&
         ux+x0>=20 && ux+x0<620 && uy+y0>=30 && uy+y0<460) {
      Print("TTF_MOVE_CAND:%d,%d,%d,%d,%d,%d,%d\\n",ti,tu->row,tu->col,
            ToI64(tx+x0),ToI64(ty+y0),ToI64(ux+x0),ToI64(uy+y0));
      found=TRUE;
     }
    }
   }
  }
 }
 if (!found) Print("TTF_MOVE_CAND:NONE\\n");'''
old_move = '\t\t\t\t\t\tUnitMove(tmpu, arg1, arg2);\n\t\t\t\t\t\tbreak;'
new_move = ('\t\t\t\t\t\told_r=tmpu->row; old_c=tmpu->col;\n'
            '\t\t\t\t\t\tUnitMove(tmpu, arg1, arg2);\n'
            '\t\t\t\t\t\tPrint("TTF_MOVE_RESULT:%d,%d,%d,%d\\n",old_r,old_c,tmpu->row,tmpu->col);\n'
            '\t\t\t\t\t\tbreak;')
if source.count(old) != 1 or source.count(old_move) != 1:
    raise SystemExit('ToTheFront source differs from the expected candidate')
source = source.replace(old, new, 1).replace(old_move, new_move, 1)
if a.direct_fire:
    direct_old = '''U0 PlayerDirect()
{
\tUnit *tmpu = NULL, *target;
\tI64   message_code, arg1, arg2;

\tViewPlayerSet(cur_player);'''
    direct_new = '''U0 PlayerDirect()
{
 Unit *tmpu = NULL, *target, *attacker, *opponent;
 I64 message_code, arg1, arg2, ai, ei, di, rr, cc;
 F64 ax, ay, bx, by;
 Bool found=FALSE;
 ViewPlayerSet(cur_player);
 for (ai=0; ai<UNITS_NUM && !found; ai++) {
  attacker=&units[cur_player][ai];
  if (attacker->life>0 && !attacker->fired && !attacker->indirect_fire && attacker->range>0) {
   for (di=0; di<6 && !found; di++) {
    rr=attacker->row; cc=attacker->col; Toward(&rr,&cc,di);
    if (rr>=0 && rr<map_rows && cc>=0 && cc<map_cols && !UnitFind(rr,cc)) {
     RowCol2XY(&ax,&ay,attacker->row,attacker->col); RowCol2XY(&bx,&by,rr,cc);
     if (ax+x0>=20 && ax+x0<620 && ay+y0>=30 && ay+y0<460 &&
         bx+x0>=20 && bx+x0<620 && by+y0>=30 && by+y0<460) {
      for (ei=0; ei<UNITS_NUM && !found; ei++) {
       opponent=&units[enemy_player][ei];
       if (opponent->life>0) {
        opponent->row=rr; opponent->col=cc;
        opponent->vis[0]=0; opponent->vis[1]=0;
        Print("TTF_FIRE_CAND:%d,%d,%d,%d,%d,%d,%d\\n",cur_player,ai,ei,
              ToI64(ax+x0),ToI64(ay+y0),ToI64(bx+x0),ToI64(by+y0));
        found=TRUE;
       }
      }
     }
    }
   }
  }
 }
 if (!found) Print("TTF_FIRE_CAND:NONE\\n");'''
    direct_check = '''\t\t\t\t\t\tVisRecalc(VR_UPDATE_FRIENDLY_UNIT, tmpu);
\t\t\t\t\t}
\t\t\t\t}
\t\t\t\ttmpu = NULL;'''
    direct_report = '''\t\t\t\t\t\tVisRecalc(VR_UPDATE_FRIENDLY_UNIT, tmpu);
                        if (target)
                            Print("TTF_FIRE_RESULT:%d,%d,%d,%d\\n",cur_player,tmpu->fired,
                                  target->vis[0],target->vis[1]);
                        else
                            Print("TTF_FIRE_RESULT:%d,%d,-1,-1\\n",cur_player,tmpu->fired);
\t\t\t\t\t}
\t\t\t\t}
\t\t\t\ttmpu = NULL;'''
    if source.count(direct_old) != 1 or source.count(direct_check) != 1:
        raise SystemExit('ToTheFront direct-fire source differs from the expected candidate')
    source = source.replace(direct_old, direct_new, 1).replace(direct_check, direct_report, 1)
a.output.parent.mkdir(parents=True, exist_ok=True)
clone = subprocess.run(['/bin/cp', '-c', str(a.disk), str(a.output)],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
if clone.returncode:
    shutil.copyfile(a.disk, a.output)
with tempfile.TemporaryDirectory(prefix='tothefront-fixture-') as tmp:
    fixture = pathlib.Path(tmp) / 'Human.ZC'
    fixture.write_text(source, encoding='utf-8')
    subprocess.run(['python3', str(ROOT/'scripts/source-volume.py'), 'put',
                    str(a.output), str(fixture), 'Apps/ToTheFront/AIs/Human.ZC'], check=True)
print('Prepared a disposable movement-reporting disk:', a.output)
