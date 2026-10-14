WsNest : MultiOutUGen {
	// All inputs but init are read once per top-level waveset; init is read once at start.
	*ar { |r = 0.9, lo = 2, hi = 20, amp = 0.5, shape = 0, map = 0, depth = 2, branch = 3, spread = 1, contour = 0, skewDepth = 0, init = 0.3, balance = 0, mul = 1, add = 0|
		^this.multiNew('audio', 1, r, lo, hi, amp, shape, map, depth, branch, spread, contour, skewDepth, init, balance, 1).madd(mul, add)
	}

	// [sig, trig]: trig fires at the start of every `every`-th top-level waveset.
	*arTrig { |r = 0.9, lo = 2, hi = 20, amp = 0.5, shape = 0, map = 0, depth = 2, branch = 3, spread = 1, contour = 0, skewDepth = 0, init = 0.3, balance = 0, every = 1|
		^this.multiNew('audio', 2, r, lo, hi, amp, shape, map, depth, branch, spread, contour, skewDepth, init, balance, every)
	}

	init { |numOuts ... theInputs|
		inputs = theInputs;
		^this.initOutputs(numOuts, rate)
	}
}
