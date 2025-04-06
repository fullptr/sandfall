#include "player.hpp"
#include "world.hpp"

namespace sand {

void contact_listener::PreSolve(b2Contact* contact, const b2Manifold* impulse)  {
    //if (contact->GetFixtureA() == d_fixture || contact->GetFixtureB() == d_fixture) {
    //    contact->ResetFriction();
    //}
}

void contact_listener::BeginContact(b2Contact* contact) {
    //if (contact->GetFixtureA() == d_footSensor) {
    //    d_floors.insert(contact->GetFixtureB());
    //}
    //if (contact->GetFixtureB() == d_footSensor) {
    //    d_floors.insert(contact->GetFixtureA());
    //}
    //if (contact->GetFixtureA() == d_left_sensor || contact->GetFixtureB() == d_left_sensor) {
    //    ++d_num_left_contacts;
    //}
    //if (contact->GetFixtureA() == d_right_sensor || contact->GetFixtureB() == d_right_sensor) {
    //    ++d_num_right_contacts;
    //}
}

void contact_listener::EndContact(b2Contact* contact) {
    //if (contact->GetFixtureA() == d_footSensor) {
    //    d_floors.erase(contact->GetFixtureB());
    //}
    //if (contact->GetFixtureB() == d_footSensor) {
    //    d_floors.erase(contact->GetFixtureA());
    //}
    //if (contact->GetFixtureA() == d_left_sensor || contact->GetFixtureB() == d_left_sensor) {
    //    --d_num_left_contacts;
    //}
    //if (contact->GetFixtureA() == d_right_sensor || contact->GetFixtureB() == d_right_sensor) {
    //    --d_num_right_contacts;
    //}
}

}