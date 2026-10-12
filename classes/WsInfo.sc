WsInfo : MultiOutUGen {
	// [freq, peak, skew, trig], updated each time an input waveset ends.
	*ar { |in, thresh = 0|
		^this.multiNew('audio', in, thresh)
	}

	*kr { |in, thresh = 0|
		^this.multiNew('control', in, thresh)
	}

	init { |... theInputs|
		inputs = theInputs;
		^this.initOutputs(4, rate)
	}
}
