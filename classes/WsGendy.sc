WsGendy : MultiOutUGen {
	// All inputs are read once per waveset; demand-rate inputs advance one value per waveset.
	*ar { |minFreq = 80, maxFreq = 800, freqStep = 0.2, ampStep = 0.1, knum = 8, amp = 0.5, skew = 0.5, interp = 0, balance = 0, mul = 1, add = 0|
		^this.multiNew('audio', 1, minFreq, maxFreq, freqStep, ampStep, knum, amp, skew, interp, balance, 1).madd(mul, add)
	}

	// [sig, trig]: trig fires at the start of every `every`-th waveset.
	*arTrig { |minFreq = 80, maxFreq = 800, freqStep = 0.2, ampStep = 0.1, knum = 8, amp = 0.5, skew = 0.5, interp = 0, balance = 0, every = 1|
		^this.multiNew('audio', 2, minFreq, maxFreq, freqStep, ampStep, knum, amp, skew, interp, balance, every)
	}

	init { |numOuts ... theInputs|
		inputs = theInputs;
		^this.initOutputs(numOuts, rate)
	}
}
