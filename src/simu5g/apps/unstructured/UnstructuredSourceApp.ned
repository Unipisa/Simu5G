//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.apps.unstructured;

//
// An ~UnstructuredApp that only sends
//
module UnstructuredSourceApp extends UnstructuredApp
{
    parameters:
        sink.typename = "";
}
