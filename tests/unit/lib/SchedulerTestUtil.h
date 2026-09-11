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

#ifndef TESTS_UNIT_LIB_SCHEDULERTESTUTIL_H_
#define TESTS_UNIT_LIB_SCHEDULERTESTUTIL_H_

#include <charconv>
#include <cmath>
#include <iostream>
#include <string>

#include <omnetpp.h>

namespace simu5g {
namespace unittest {

/**
 * Checker for properties that have no reference formula to be graded against:
 * the scheduler tests assert the specification's qualitative rules (equal
 * service, strict priority order) and the model's own stated contracts (byte
 * accounting, ratio invariance), all of which are decided here in the test.
 * This is the "modelchecks" category of the path-loss suite, factored out:
 * every check prints a verdict line and the summary() count is what the .test
 * file matches on.
 */
class Checker
{
  private:
    int checks_ = 0;
    int failures_ = 0;

    // shortest representation that reads back as the same double
    static std::string num(double d)
    {
        char buf[32];
        auto [end, ec] = std::to_chars(buf, buf + sizeof(buf), d);
        return std::string(buf, end);
    }

  public:
    /**
     * Assert a condition the test computed itself; detail is appended to the
     * verdict line either way, so a failure names the offending numbers.
     */
    void check(const std::string& what, bool ok, const std::string& detail = "")
    {
        checks_++;
        if (!ok)
            failures_++;
        std::cout << (ok ? "model ok: " : "model FAILED: ") << what
                  << (detail.empty() ? "" : " -- " + detail) << std::endl;
    }

    /**
     * Assert an exact value, for byte-accounting contracts where the model's
     * arithmetic is integral and a tolerance would only mask an off-by-header.
     */
    void expectValue(const std::string& what, long long actual, long long expected)
    {
        check(what, actual == expected,
                "actual=" + std::to_string(actual) + " expected=" + std::to_string(expected));
    }

    /**
     * Assert a value within an absolute tolerance.
     */
    void expectNear(const std::string& what, double actual, double expected, double tolerance)
    {
        check(what, std::fabs(actual - expected) <= tolerance,
                "actual=" + num(actual) + " expected=" + num(expected) + " tol=" + num(tolerance));
    }

    /**
     * Verify that a call is rejected with an error. Takes the call as a lambda.
     */
    template<typename Fn>
    void expectRejected(const std::string& what, Fn fn)
    {
        checks_++;
        try {
            fn();
            failures_++;
            std::cout << "model FAILED: " << what << ": expected an error, but none was thrown" << std::endl;
        }
        catch (omnetpp::cRuntimeError& e) {
            std::cout << "model ok: " << what << " threw as expected" << std::endl;
        }
    }

    void summary() const
    {
        std::cout << "modelchecks=" << checks_ << " modelfailures=" << failures_ << std::endl;
    }
};

} //namespace unittest
} //namespace simu5g

#endif
