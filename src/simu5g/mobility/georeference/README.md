# Geographic Reference System

This folder contains the `GeographicReferenceSystem` module, which is the bridge between:

- geographic positions expressed on the Earth (`WGS84`)
- Earth-centered Cartesian coordinates (`ECEF`)
- the local Cartesian coordinates used by OMNeT++/INET inside the simulation (`Coord`)

The module is especially useful for non-terrestrial network scenarios, where some nodes are naturally described by latitude, longitude, and altitude rather than by a purely local `(x, y, z)` position.

## Why this module exists

OMNeT++ and INET simulate mobility and radio interactions in a Cartesian 3D space. That is convenient for the simulator, but it is not how satellite positions, ground station locations, or map-based scenarios are usually specified.

Real-world positions are typically given as:

- latitude
- longitude
- altitude

Those quantities are not directly suitable for simulation geometry:

- latitude and longitude are angular coordinates on an ellipsoid, not linear distances
- the physical distance represented by one degree changes with latitude
- Earth curvature matters for large-scale scenarios, especially for satellites

The `GeographicReferenceSystem` solves this by defining a local simulation frame anchored to a real geographic point. In other words, it says:

`this OMNeT++ point corresponds to this real point on Earth`

Once that anchor is defined, positions given in geographic coordinates can be converted into simulator coordinates consistently.

## The three coordinate systems

### WGS84

`WGS84` stands for *World Geodetic System 1984*. It is the standard Earth reference model used by GPS and by many geographic datasets.

In this context, a WGS84 position is represented as:

- latitude: angular position north/south of the equator
- longitude: angular position east/west of the Greenwich meridian
- altitude: height above the WGS84 reference ellipsoid

WGS84 is the right coordinate system when:

- a position comes from maps, GPS, orbital tools, or TLE-based post-processing
- users want to configure nodes using real places on Earth
- logs or interfaces should remain human-readable

WGS84 is *not* a Cartesian system. Two points expressed in latitude/longitude cannot be safely subtracted component-wise to obtain a local displacement in meters.

### ECEF

`ECEF` stands for *Earth-Centered, Earth-Fixed*.

This is a 3D Cartesian coordinate system:

- origin at the Earth's center of mass
- `x`, `y`, and `z` axes fixed to the Earth
- coordinates expressed in meters

ECEF is useful because it converts a geographic position on the WGS84 ellipsoid into a global Cartesian point. That makes it a good intermediate representation for geometric calculations on a curved Earth.

Compared to WGS84:

- WGS84 is intuitive and geographic
- ECEF is Cartesian and geometric

Compared to OMNeT++ coordinates:

- ECEF is global and Earth-centered
- OMNeT++ coordinates are local and scenario-centered

In this module, the reference geographic point is converted to ECEF once and stored as `referenceEcefCoord_`.

### OMNeT++ / INET coordinates

OMNeT++ and INET represent positions with `inet::Coord`, a local Cartesian coordinate:

- `x`: local horizontal axis
- `y`: local vertical-on-the-canvas axis used by the simulator scene
- `z`: local altitude axis

These coordinates are:

- linear, in meters
- local to the scenario
- convenient for mobility, distance, radio, and visualization code

They are not tied to the Earth by themselves. Without a geographic reference, `(1000, 2000, 0)` is just a point in the simulation world, not a known place on Earth.

## Why all three are needed

Each system has a different role:

- `WGS84` is how we describe real places and satellite sub-points
- `ECEF` is a mathematically convenient global Cartesian representation of those places
- `OMNeT++` coordinates are what the simulator actually uses to place and move modules

Using only one of them would be problematic:

- only WGS84: not suitable for local simulation geometry and Cartesian mobility
- only ECEF: mathematically correct, but inconvenient for user configuration and not centered on the scenario
- only OMNeT++ coordinates: easy for the simulator, but disconnected from real Earth positions

The module therefore uses:

1. WGS84 for input and interpretation
2. ECEF for Earth-aware geodetic conversion
3. OMNeT++ coordinates for the actual simulated positions

## What the module does

The `GeographicReferenceSystem` defines one anchor between the real world and the simulator world.

It stores:

- `referenceOmnetCoord_`: the OMNeT++ point configured by parameters `x`, `y`, `z`
- `referenceWgs84_`: the geographic point configured by `referenceLatitude`, `referenceLongitude`, `referenceAltitude`
- `referenceEcefCoord_`: the ECEF version of that same geographic point

During initialization, the module:

1. reads the OMNeT++ anchor point
2. reads the WGS84 anchor point
3. converts the WGS84 anchor into ECEF with `GeographicLib::Geocentric`
4. initializes a local tangent frame with `GeographicLib::LocalCartesian`

That local tangent frame is centered on the configured WGS84 reference point.

## The local tangent frame used here

`GeographicLib::LocalCartesian` builds a local coordinate system around the reference point. Its axes are:

- `East`
- `North`
- `Up`

This is often called an `ENU` frame.

Given any WGS84 point, the module computes its local displacement relative to the reference point as:

- how far east it is
- how far north it is
- how far up it is

This is exactly what is needed to place the point in a local simulation space.

## The conversion implemented by `omnetFromWgs84()`

The key method in this module is:

```cpp
inet::Coord omnetFromWgs84(const inet::GeoCoord& wgs84Coord) const;
```

Its behavior is:

1. convert the input WGS84 point into local `east`, `north`, `up` coordinates with respect to the configured reference point
2. shift those local coordinates by the configured OMNeT++ anchor `(x, y, z)`
3. return the resulting OMNeT++ position

The exact mapping used by the code is:

```text
omnet.x = referenceOmnet.x + east
omnet.y = referenceOmnet.y - north
omnet.z = referenceOmnet.z + up
```

The sign inversion on `y` is intentional.

The local geographic frame uses positive `north`, while OMNeT++ scenes commonly use a screen-oriented convention where increasing `y` goes downward. Mapping `north` to `-y` makes the simulated layout consistent with the expected visual orientation of map-like backgrounds and scenario displays.

So, in this module:

- east becomes positive `x`
- north becomes negative `y`
- up becomes positive `z`

## How the module is configured

The NED parameters are:

- `x`, `y`, `z`: the OMNeT++ coordinate of the anchor point
- `referenceLatitude`, `referenceLongitude`, `referenceAltitude`: the real WGS84 coordinate associated with that anchor

Example from `simulations/nr/ntn_smoke/omnetpp.ini`:

```ini
*.geoRef.x = 0m
*.geoRef.y = 0m
*.geoRef.z = 0m
*.geoRef.referenceLatitude = 43.7deg
*.geoRef.referenceLongitude = 10.4deg
*.geoRef.referenceAltitude = 0m
```

This means:

- the OMNeT++ origin `(0, 0, 0)` is tied to the geographic point `(43.7 deg, 10.4 deg, 0 m)`
- every WGS84 position converted by the module will be placed relative to that real-world anchor

## How the module is used in the simulator

The module is instantiated as a regular submodule in the network, for example:

```ned
geoRef: GeographicReferenceSystem {
    @display("p=50,345;is=s");
}
```

Other modules then find it at runtime through `GeographicReferenceSystemAccess`, which recursively searches the simulation hierarchy.

This means the geographic reference system acts as a shared service for all modules that need geographic-to-local conversion.

## Usage with GEO satellites

`GeoSatMobility` uses the reference system in a simple way.

The user configures the satellite position in WGS84:

- `initialLatitude`
- `initialLongitude`
- `initialAltitude`

During initialization, `GeoSatMobility`:

1. obtains the `GeographicReferenceSystem`
2. builds a `GeoCoord` from the configured geographic parameters
3. calls `referenceSystem_->omnetFromWgs84(...)`
4. stores the returned `inet::Coord` as the satellite position in the simulator

So the GEO satellite is specified geographically but simulated locally.

## Usage with LEO satellites

`LeoSatMobility` uses the same reference system, but its geographic position is not directly configured by latitude and longitude parameters.

Instead, the flow is:

1. orbital propagation is performed with `SGP4` using TLE data
2. the resulting position is converted from `TEME` to an Earth-fixed frame in the `ITRF` family
3. that Earth-fixed Cartesian position is converted to latitude, longitude, and altitude on the WGS84 ellipsoid
4. the resulting WGS84 coordinate is passed to `referenceSystem_->omnetFromWgs84(...)`
5. the resulting OMNeT++ coordinate becomes the current satellite position

So even when the mobility starts from orbital mechanics rather than from a static geographic configuration, the last step is still the same:

`WGS84 -> local OMNeT++ coordinates through GeographicReferenceSystem`

## Computing distances

Distance computation depends on what the distance will be used for.

The most important distinction is between:

- a `local simulation distance`, used because nodes already live in the OMNeT++ frame
- a `physical Earth-referenced distance`, used because propagation or geometry should reflect the actual separation of points around the Earth

There is no single "best" coordinate system for every case:

- use OMNeT++ local coordinates when you need the distance between already placed nodes in the simulator
- use WGS84 together with an Earth-aware conversion when you need a physically meaningful distance between real geographic positions
- use ECEF as the Cartesian representation behind that Earth-aware computation

### Case 1: distance inside the local simulation space

If two nodes already have valid `inet::Coord` positions in the scenario, the simplest distance is the Euclidean distance in OMNeT++ space.

This is appropriate when:

- both positions are already expressed in the same local frame
- the scenario covers a limited area around the geographic reference point
- you are working with mobility, visualization, or propagation code that already uses OMNeT++ coordinates

Conceptually, this is:

```text
distance = |p1_omnet - p2_omnet|
```

This is the natural choice after `GeographicReferenceSystem` has already converted the positions, because both nodes now live in the same local ENU-derived frame.

This is also how much of the existing terrestrial propagation code in Simu5G operates: channel and PHY components commonly use `Coord::distance()` on the radio positions already available in the simulator.

### Case 2: distance for radio propagation over Earth-referenced links

If the distance is meant to represent the actual propagation path between geographic positions, you should not compute it by subtracting latitudes and longitudes directly.

Instead, the Earth-aware approach is:

1. start from WGS84 coordinates
2. convert each point to ECEF
3. compute the Euclidean distance between the two ECEF points

This gives the straight-line 3D separation, often called the slant range.

This is exactly the geometry Simu5G now builds by converting the two WGS84 endpoints with
`ecefFromWgs84()` in `src/simu5g/common/GeoUtils.cc` and then taking the Euclidean distance
between the resulting ECEF points.

That function:

- takes two geographic points plus their altitudes
- converts them to ECEF using `GeographicLib::Geocentric` with the WGS84 ellipsoid
- returns the straight-line 3D distance between the two resulting Cartesian points

For radio propagation, this is the right choice when:

- one or both endpoints are still in geographic form
- satellite links are involved
- the endpoints are far apart
- propagation delay or free-space path loss should be based on the actual 3D Earth-referenced separation

In NTN scenarios, this is usually the most physically meaningful distance for link-budget purposes.

### Case 3: which one should I choose for propagation?

For propagation calculations, the answer depends on the propagation model.

Use OMNeT++ local coordinates when:

- the channel model is already formulated in terms of the simulator's radio positions
- all nodes share the same local frame
- the local Cartesian approximation is acceptable for the scenario scale
- you want behavior consistent with the existing terrestrial channel models in Simu5G

Use WGS84 plus ECEF conversion when:

- the propagation distance should represent the actual physical separation in Earth-referenced space
- your endpoints come from geographic or orbital data
- the link spans large distances
- you are modeling satellite-to-ground or satellite-to-satellite propagation

In practice:

- `local OMNeT++ distance` is usually sufficient for terrestrial or locally bounded scenarios
- `WGS84 -> ECEF -> distance` is usually preferable for NTN propagation, especially when the path loss or delay should follow the true slant range

### Relation to the geographic reference system

`GeographicReferenceSystem` itself does not provide a dedicated distance API. Its job is to create a consistent local frame and convert `WGS84` positions into `inet::Coord`.

Once the conversion has been done, distances can be computed in OMNeT++ coordinates because all nodes share the same local reference.

However, for radio propagation over large Earth-referenced links, it is often better to compute the distance from the original geographic data through ECEF, because:

- the OMNeT++ frame is local by design
- ECEF remains globally valid
- the relevant radio quantity may be the true 3D slant range rather than the distance inside a local map projection

### Practical rule of thumb

Use this rule:

- if the question is "how far apart are these nodes in the current simulation frame?", use OMNeT++ coordinates
- if the question is "what distance should drive propagation delay or free-space path loss on a geographic/NTN link?", use WGS84 inputs and compute the distance through ECEF

For NTN scenarios, especially satellite-ground distances, the second option is usually the more robust one when the source data is geographic and the propagation model is meant to reflect the actual Earth-referenced geometry.

## Why this is important for NTN simulations

For terrestrial-only scenarios, it is often enough to place nodes directly in a local Cartesian plane.

For NTN scenarios, that is much less convenient because:

- satellites are naturally described in global coordinates
- ground stations may correspond to real places
- Earth curvature matters
- orbital propagators and geographic datasets do not operate in OMNeT++ coordinates

The `GeographicReferenceSystem` lets the simulator combine:

- realistic Earth-referenced inputs
- standard geodetic conversions from GeographicLib
- the local Cartesian coordinates expected by INET mobility and radio models

## Practical interpretation

You can think of the module as creating a statement like:

`the real Earth location (lat0, lon0, alt0) is represented in the simulator by the point (x0, y0, z0)`

Once that is fixed:

- nearby geographic points become nearby simulation points
- altitude is preserved as local `z`
- east-west and north-south displacements are converted into local Cartesian offsets

This gives the simulator a consistent local view of a small portion of the Earth, while still allowing nodes such as satellites to originate from global geographic or orbital data.

## Summary

`GeographicReferenceSystem` is the module that anchors Simu5G's local simulation space to the real Earth.

- `WGS84` provides the geographic description of positions
- `ECEF` provides a global Cartesian Earth-centered representation
- `OMNeT++ coordinates` provide the local Cartesian positions used by the simulator

The module connects these worlds by centering a local East-North-Up frame on a configurable WGS84 reference point and mapping that local frame into OMNeT++ coordinates.

That is why GEO and LEO satellite mobility modules can work with real-world geographic data while still participating in the standard INET/OMNeT++ mobility and radio pipeline.
