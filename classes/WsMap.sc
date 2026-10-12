WsMap : UGen {
	// All inputs but init are read once per waveset; init is read once at start.
	*ar { |r = 0.9, lo = 60, hi = 1200, amp = 0.5, shape = 0, map = 0, skewDepth = 0, ampDepth = 0, init = 0.3, balance = 0, mul = 1, add = 0|
		^this.multiNew('audio', r, lo, hi, amp, shape, map, skewDepth, ampDepth, init, balance).madd(mul, add)
	}
}
