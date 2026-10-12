WsGendy : UGen {
	// All inputs are read once per waveset; demand-rate inputs advance one value per waveset.
	*ar { |minFreq = 80, maxFreq = 800, freqStep = 0.2, ampStep = 0.1, knum = 8, amp = 0.5, skew = 0.5, interp = 0, mul = 1, add = 0|
		^this.multiNew('audio', minFreq, maxFreq, freqStep, ampStep, knum, amp, skew, interp).madd(mul, add)
	}
}
