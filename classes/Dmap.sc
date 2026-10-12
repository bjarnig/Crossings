Dmap : DUGen {
	// Each pull steps the map once; init is read on start and reset.
	*new { |r = 0.9, map = 0, init = 0.3, axis = 0, hyst = 0, length = inf|
		^this.multiNew('demand', length, r, map, init, axis, hyst)
	}
}
