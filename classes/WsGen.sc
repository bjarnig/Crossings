WsGen : MultiOutUGen {
	// All inputs are read once per waveset; demand-rate inputs advance one value per waveset.
	*ar { |freq = 220, amp = 0.5, shape = 0, skew = 0.5, repeats = 1, glide = 0, balance = 0, mul = 1, add = 0|
		^this.multiNew('audio', 1, freq, amp, shape, skew, repeats, glide, balance, 1).madd(mul, add)
	}

	// [sig, trig]: trig fires at the start of every `every`-th waveset.
	*arTrig { |freq = 220, amp = 0.5, shape = 0, skew = 0.5, repeats = 1, glide = 0, balance = 0, every = 1|
		^this.multiNew('audio', 2, freq, amp, shape, skew, repeats, glide, balance, every)
	}

	init { |numOuts ... theInputs|
		inputs = theInputs;
		^this.initOutputs(numOuts, rate)
	}
}
