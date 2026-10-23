"""Protocol and mapping checks without serial devices or the robot SDK."""
import importlib.util
from pathlib import Path
import numpy as np
import pytest

spec = importlib.util.spec_from_file_location('zhonglin', Path(__file__).parents[1] / 'python/src/rcore_peripherals/zhonglin.py')
z = importlib.util.module_from_spec(spec)
spec.loader.exec_module(z)


def test_mapping_home_and_relative_j5():
    np.testing.assert_allclose(z.master_pwm_to_i3_joints(z.HOME_PWM), np.radians(z.I3_HOME_DEG))
    pwm = z.HOME_PWM.copy()
    pwm[4] += 10
    q = z.master_pwm_to_i3_joints(pwm, reference_pwm=z.HOME_PWM, reference_q=np.radians(z.I3_HOME_DEG))
    assert np.degrees(q[4]) == pytest.approx(2.7)


def test_partial_sweep_never_reuses_last_sample():
    reader = z.ZhonglinReader.__new__(z.ZhonglinReader)
    reader.read_pwm = lambda servo_id, retries: 1500 if servo_id != 3 else None
    reader.last_pwm = np.ones(6) * 1600
    assert reader.read_all() is None


def test_protocol_rejects_wrong_id_and_out_of_range():
    reader = z.ZhonglinReader.__new__(z.ZhonglinReader)
    for reply in (b'#001P1500!', b'#000P9999!', b'garbage'):
        reader.command = lambda *args, **kwargs: reply
        assert reader.read_pwm(0, retries=1) is None
    reader.command = lambda *args, **kwargs: b'#000P1500!'
    assert reader.read_pwm(0, retries=1) == 1500


def test_invalid_input_and_result_rejected():
    for values in ([1500]*5, [float('nan')]*6, [2501]*6):
        with pytest.raises(ValueError):
            z.master_pwm_to_i3_joints(values)
    assert not z.result_ok(None)
    assert not z.result_ok(object())
    assert not z.result_ok({'success': False})
    assert z.result_ok({'success': True})


@pytest.mark.parametrize('failure', ['stale', 'reject'])
def test_bridge_stops_on_input_timeout_or_rejection(monkeypatch, failure):
    import sys
    import types
    bridge_path = Path(__file__).resolve().parents[1] / 'python/app/app_peripherals_zhonglin_i3_bridge.py'
    helper = z
    monkeypatch.setitem(sys.modules, 'rcore_peripherals', types.ModuleType('rcore_peripherals'))
    monkeypatch.setitem(sys.modules, 'rcore_peripherals.zhonglin', helper)
    spec = importlib.util.spec_from_file_location('test_bridge', bridge_path)
    bridge = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(bridge)
    calls = []
    class Reader:
        def __init__(self, *args): self.count = 0
        def read_all(self):
            self.count += 1
            return helper.HOME_PWM.copy() if self.count == 1 or failure == 'reject' else None
        def close(self): calls.append('close')
    class Robot:
        def Initialize(self, ip): return True
        def GetState(self): return types.SimpleNamespace(positions=np.radians(helper.I3_HOME_DEG))
        def Enable(self): return True
        def ServoJ(self, q): calls.append('servo'); return False
        def StopMotion(self): calls.append('stop'); return True
        def Shutdown(self): calls.append('shutdown')
    monkeypatch.setattr(bridge, 'ZhonglinReader', Reader)
    monkeypatch.setattr(bridge.signal, 'signal', lambda *args: None)
    clock = iter(i * 0.01 for i in range(100))
    monkeypatch.setattr(bridge.time, 'monotonic', lambda: next(clock))
    monkeypatch.setattr(bridge.time, 'sleep', lambda _: None)
    monkeypatch.setitem(sys.modules, 'rcore_sdk', types.SimpleNamespace(Robot=Robot, JointPositions=lambda q: q))
    monkeypatch.setattr(sys, 'argv', ['bridge', '--execute', '--robot-ip', 'test', '--keep-master-torque'])
    with pytest.raises(RuntimeError, match='stale' if failure == 'stale' else 'ServoJ rejected'):
        bridge.main()
    assert calls[-3:] == ['stop', 'shutdown', 'close']
    assert calls.count('servo') == (1 if failure == 'reject' else 0)
