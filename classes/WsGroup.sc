WsGroup : MultiOutUGen {
	// All inputs are read once per group; demand-rate inputs advance one value per group.
	*ar { |freq = 110, amp = 0.5, shape = 0, halves = 3, tilt = 0, decay = 0, flip = 0, balance = 0, mul = 1, add = 0|
		^this.multiNew('audio', 1, freq, amp, shape, halves, tilt, decay, flip, balance, 1).madd(mul, add)
	}

	// [sig, trig]: trig fires at the start of every `every`-th group.
	*arTrig { |freq = 110, amp = 0.5, shape = 0, halves = 3, tilt = 0, decay = 0, flip = 0, balance = 0, every = 1|
		^this.multiNew('audio', 2, freq, amp, shape, halves, tilt, decay, flip, balance, every)
	}

	init { |numOuts ... theInputs|
		inputs = theInputs;
		^this.initOutputs(numOuts, rate)
	}
}
