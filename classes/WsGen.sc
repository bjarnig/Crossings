WsGen : UGen {
	// All inputs are read once per waveset; demand-rate inputs advance one value per waveset.
	*ar { |freq = 220, amp = 0.5, shape = 0, skew = 0.5, repeats = 1, glide = 0, balance = 0, mul = 1, add = 0|
		^this.multiNew('audio', freq, amp, shape, skew, repeats, glide, balance).madd(mul, add)
	}
}
