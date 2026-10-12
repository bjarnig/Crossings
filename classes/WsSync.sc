WsSync : MultiOutUGen {
	// Generated wavesets on the input's zero crossings; inputs are read once per input waveset.
	*ar { |in, amp = 0.5, shape = 0, skew = 0.5, count = 1, skewFollow = 0, ampFollow = 0, thresh = 0, balance = 0, mul = 1, add = 0|
		^this.multiNew('audio', 1, in, amp, shape, skew, count, skewFollow, ampFollow, thresh, balance, 1).madd(mul, add)
	}

	// [sig, trig]: trig fires at the start of every `every`-th generated waveset.
	*arTrig { |in, amp = 0.5, shape = 0, skew = 0.5, count = 1, skewFollow = 0, ampFollow = 0, thresh = 0, balance = 0, every = 1|
		^this.multiNew('audio', 2, in, amp, shape, skew, count, skewFollow, ampFollow, thresh, balance, every)
	}

	init { |numOuts ... theInputs|
		inputs = theInputs;
		^this.initOutputs(numOuts, rate)
	}
}
