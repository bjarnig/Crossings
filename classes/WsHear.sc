WsHear : MultiOutUGen {
	// All inputs but in, thresh and init are read once per generated waveset.
	*ar { |in, r = 0.9, lo = 60, hi = 1200, amp = 0.5, shape = 0, map = 0, couple = 0.3, thresh = 0.01, skewDepth = 0, ampDepth = 0, init = 0.3, balance = 0, hyst = 0, mul = 1, add = 0|
		^this.multiNew('audio', 1, in, r, lo, hi, amp, shape, map, couple, thresh, skewDepth, ampDepth, init, balance, hyst, 1).madd(mul, add)
	}

	// [sig, trig]: trig fires at the start of every `every`-th waveset.
	*arTrig { |in, r = 0.9, lo = 60, hi = 1200, amp = 0.5, shape = 0, map = 0, couple = 0.3, thresh = 0.01, skewDepth = 0, ampDepth = 0, init = 0.3, balance = 0, hyst = 0, every = 1|
		^this.multiNew('audio', 2, in, r, lo, hi, amp, shape, map, couple, thresh, skewDepth, ampDepth, init, balance, hyst, every)
	}

	init { |numOuts ... theInputs|
		inputs = theInputs;
		^this.initOutputs(numOuts, rate)
	}
}
