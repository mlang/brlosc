BrlOSC {
	classvar ports;
	var keyFunc, errFunc, host, pid, <connected = false, <tty, bridge, oscFuncs;
	*initClass {
		ports = StackNumberAllocator.new(27500, 27599);
	}
	*new {| keyFunc, errFunc, host |
		^super.newCopyArgs(keyFunc, errFunc, host).init;
	}
	init {
		bridge = NetAddr.new("127.0.0.1", ports.alloc);
		pid = [
			"BrlOSC", NetAddr.langPort.asString, bridge.port.asString, host ? ""
		].unixCmd({|res|
			ports.free(bridge.port);
			ShutDown.remove(this);
			super.free;
		});
		oscFuncs = [
			OSCFunc({|msg|
				errFunc.value(msg[1], this)
			}, '/error', bridge),
			OSCFunc({ this.changed(\connected, connected = true) }, '/connected', bridge),
			OSCFunc({|msg| this.changed(\acquired, tty = msg[1]) }, '/tty', bridge),
			OSCFunc({|msg|
				keyFunc.value(msg[1], msg[2], msg[3], this);
			}, '/key', bridge)
		];
		ShutDown.add(this)
	}
	grab {|tty|
		if(connected, {
			if (tty.isNil, {
				bridge.sendMsg('/grab')
			}, {
				bridge.sendMsg('/grab', tty.asInteger)
			})
		})
	}
	write {| what |
		if (connected, { bridge.sendMsg('/write', what) })
	}
	release {
		bridge.sendMsg('/leave');
	}
	doOnShutDown { this.free }
	free {
		oscFuncs.do(_.free);
		bridge.sendMsg('/quit');
	}
}
