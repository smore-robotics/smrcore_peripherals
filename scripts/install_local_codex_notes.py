#!/usr/bin/env python3
"""Install the current user's local Codex release-note timer."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def quote(value):
    return '"'+str(value).replace('\\','\\\\').replace('"','\\"').replace('%','%%')+'"'


if __name__ == '__main__':
    units = Path.home()/'.config/systemd/user'
    units.mkdir(parents=True,exist_ok=True)
    (units/'peripherals-codex-notes.service').write_text(f'''[Unit]
Description=Generate peripheral release-note drafts using local ChatGPT login

[Service]
Type=oneshot
ExecStart={quote(sys.executable)} {quote(ROOT/'scripts/local_codex_notes.py')}
WorkingDirectory={str(ROOT).replace("%", "%%")}
Environment=PATH=/usr/local/bin:/usr/bin:/bin
TimeoutStartSec=25min
UMask=0077
''')
    (units/'peripherals-codex-notes.timer').write_text('''[Unit]
Description=Check peripheral changes every 10 minutes

[Timer]
OnCalendar=*:0/10
Persistent=true
RandomizedDelaySec=30
Unit=peripherals-codex-notes.service

[Install]
WantedBy=timers.target
''')
    subprocess.run(['systemctl','--user','daemon-reload'],check=True)
    subprocess.run(['systemctl','--user','enable','--now','peripherals-codex-notes.timer'],check=True)
    print('Local timer enabled; inspect with systemctl --user status peripherals-codex-notes.timer')
