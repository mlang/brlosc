BrlOSC {
	classvar ports;
	var keyFunc, errFunc, pid, <tty, bridge, oscFuncs;
	*initClass {
		ports = StackNumberAllocator.new(27500, 27599);
	}
	*new {| keyFunc, errFunc |
		^super.newCopyArgs(keyFunc, errFunc).init;
	}
	init {
		bridge = NetAddr.new("127.0.0.1", ports.alloc);
		pid = [
			"BrlOSC", NetAddr.langPort.asString, bridge.port.asString
		].unixCmd({
			ports.free(bridge.port);
			super.free;
		});
		oscFuncs = [
			OSCFunc({|msg|
				errFunc.value(msg[1], this)
			}, '/error', bridge),
			OSCFunc({|msg|
				tty = msg[1]
			}, '/tty', bridge),
			OSCFunc({|msg|
				keyFunc.value(msg[1], msg[2], msg[3], this);
			}, '/key', bridge)
		];
		ShutDown.add(this)
	}
	enter {| tty |
		bridge.sendMsg('/enter', tty ? -1)
	}
	write {| what |
		bridge.sendMsg('/write', what);
	}
	leave {
		bridge.sendMsg('/leave');
		tty = nil;
	}
	doOnShutDown { this.free }
	free {
		oscFuncs.do(_.free);
		bridge.sendMsg('/quit');
	}
}
