#pragma once

#include "tree/TDetectorReadHit.h"

#include "base/types.h"
#include "base/std_ext/math.h"

#include "base/vec/vec3.h"

#include <list>
#include <memory>

namespace ant {

struct TTarget {
    struct ActivePhotons
    {

        struct Datum
        {
           TDetectorReadHit::Value_t Value;

           Datum(const TDetectorReadHit::Value_t& value):
               Value(value)
           {}

           template<class Archive>
           void serialize(Archive& archive) {
               archive(Value);
           }
           Datum() {}
        };

       std::list<Datum> Data;
       double PhotonNumber;

       ActivePhotons(double photonnumber):
           PhotonNumber(photonnumber)
       {}

       template<class Archive>
       void serialize(Archive& archive) {
           archive(PhotonNumber);
       }

       friend std::ostream& operator<<( std::ostream& s, const ActivePhotons& o) {
           s << ", Number Photons=" << o.PhotonNumber;
           for(const auto& datum : o.Data) {
               s << "=" << datum.Value;
           }
           return s;
       }
       ActivePhotons()=default;
       };

     /***struct ActivePhotons
     {

         struct Datum
         {
            unsigned Channel;
            TDetectorReadHit::Value_t Value;

            Datum(unsigned channel, const TDetectorReadHit::Value_t& value):
                Channel(channel),
                Value(value)
            {}

            template<class Archive>
            void serialize(Archive& archive) {
                archive(Channel, Value);
            }
            Datum() {}
         };

        std::list<Datum> Data;
        unsigned Channel;
        double PhotonNumber;

        ActivePhotons(unsigned channel, double photonnumber):
            Channel(channel), PhotonNumber(photonnumber)
        {}

        template<class Archive>
        void serialize(Archive& archive) {
            archive(Channel, PhotonNumber);
        }

        friend std::ostream& operator<<( std::ostream& s, const ActivePhotons& o) {
            s << "Channel =" << o.Channel << ", Number Photons=" << o.PhotonNumber;
            for(const auto& datum : o.Data) {
                s << "Channel=" << datum.Channel << "=" << datum.Value;
            }
            return s;
        }
        ActivePhotons()=default;
        };***/

    vec3 Vertex;
    double ActivePhotons;

    TTarget(const vec3& vertex = {std_ext::NaN, std_ext::NaN, std_ext::NaN}, const double& activephotons={std_ext::NaN}):
        Vertex(vertex),
        ActivePhotons(activephotons)
    {}


    template<class Archive>
    void serialize(Archive& archive) {
        archive(Vertex);
    }


    friend std::ostream& operator<<(std::ostream& s, const TTarget& o) {
        s << "Target Vertex=" << o.Vertex;
        return s;
    }

};

}
