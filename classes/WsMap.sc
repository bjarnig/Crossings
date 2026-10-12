WsMap : MultiOutUGen {
	// All inputs but init are read once per waveset; init is read once at start.
	*ar { |r = 0.9, lo = 60, hi = 1200, amp = 0.5, shape = 0, map = 0, skewDepth = 0, ampDepth = 0, init = 0.3, balance = 0, mul = 1, add = 0|
		^this.multiNew('audio', 1, r, lo, hi, amp, shape, map, skewDepth, ampDepth, init, balance, 1).madd(mul, add)
	}

	// [sig, trig]: trig fires at the start of every `every`-th waveset.
	*arTrig { |r = 0.9, lo = 60, hi = 1200, amp = 0.5, shape = 0, map = 0, skewDepth = 0, ampDepth = 0, init = 0.3, balance = 0, every = 1|
		^this.multiNew('audio', 2, r, lo, hi, amp, shape, map, skewDepth, ampDepth, init, balance, every)
	}

	init { |numOuts ... theInputs|
		inputs = theInputs;
		^this.initOutputs(numOuts, rate)
	}
}
