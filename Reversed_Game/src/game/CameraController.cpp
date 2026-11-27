#include "game/CameraController.hpp"

#include "game/Triggers.hpp"

#include <algorithm>
#include <cmath>

namespace game {

using soft3d::Fixed;
using soft3d::Vec3;
using soft3d::Matrix4;

namespace {

constexpr Vec3 worldUp{0, soft3d::fixedOne, 0};
constexpr Vec3 targetOffset{0, 0x20000, 0};

Vec3 Lerp(const Vec3& from, const Vec3& to, Fixed amount)
{
    return soft3d::Add(from, soft3d::Scale(soft3d::Subtract(to, from), amount));
}

bool Near(const Vec3& left, const Vec3& right, Fixed epsilon = 0x28f)
{
    const Vec3 delta = soft3d::Subtract(left, right);
    return soft3d::AbsFixed(delta.x) <= epsilon && soft3d::AbsFixed(delta.y) <= epsilon &&
           soft3d::AbsFixed(delta.z) <= epsilon;
}

Fixed Distance(const Vec3& left, const Vec3& right)
{
    return soft3d::Length(soft3d::Subtract(left, right));
}

Matrix4 AxisAngleMatrix(const Vec3& axis, uint16_t angle)
{
    const Fixed sine = soft3d::FixedSin(angle);
    const Fixed cosine = soft3d::FixedCos(static_cast<int16_t>(angle));
    const Fixed remainder = soft3d::SubtractFixed(soft3d::fixedOne, cosine);
    const Fixed xy = soft3d::FixedMultiply(remainder, axis.y);
    const Fixed xz = soft3d::FixedMultiply(remainder, axis.z);
    const Fixed xyx = soft3d::FixedMultiply(xy, axis.x);
    const Fixed xzx = soft3d::FixedMultiply(xz, axis.x);
    const Fixed xs = soft3d::FixedMultiply(axis.x, sine);
    const Fixed ys = soft3d::FixedMultiply(axis.y, sine);
    const Fixed zs = soft3d::FixedMultiply(axis.z, sine);
    const Fixed xx = soft3d::FixedMultiply(soft3d::FixedMultiply(remainder, axis.x), axis.x);
    Matrix4 result;
    result.values = {
        soft3d::AddFixed(xx, cosine), soft3d::SubtractFixed(xyx, zs), soft3d::AddFixed(xzx, ys), 0,
        soft3d::AddFixed(xyx, zs), soft3d::AddFixed(soft3d::FixedMultiply(xy, axis.y), cosine),
        soft3d::SubtractFixed(xzx, xs), 0,
        soft3d::SubtractFixed(xzx, ys), soft3d::AddFixed(xzx, xs),
        soft3d::AddFixed(soft3d::FixedMultiply(xz, axis.z), cosine), 0,
        0, 0, 0, soft3d::fixedOne,
    };
    return result;
}

Vec3 RotateAroundAxis(const Vec3& value, const Vec3& axis, uint16_t angle)
{
    return soft3d::Transform(AxisAngleMatrix(axis, angle), value);
}

void MoveComponent(Fixed& current, Fixed destination, Fixed delta, bool& reached)
{
    const Fixed remaining = soft3d::SubtractFixed(current, destination);
    if (soft3d::AbsFixed(delta) < soft3d::AbsFixed(remaining)) {
        current = soft3d::SubtractFixed(current, delta);
    } else {
        current = destination;
        reached = true;
    }
}

} // namespace

CameraController::CameraController()
{
    triggers.reserve(32);
    freeReference = {0, 0, soft3d::fixedOne};
    orbitTarget = target;
    orbitPosition = position;
}

void CameraController::SetServices(soft3d::Camera* newCamera, soft3d::Bsp* newBsp,
                                   soft3d::ControlMap* newControls,
                                   const soft3d::Input* newInput, uint32_t* newRandomState)
{
    camera = newCamera;
    bsp = newBsp;
    controls = newControls;
    controlInput = newInput;
    randomState = newRandomState;
}

void CameraController::Bind(soft3d::CameraSpot* newSpot, bool preservePosition)
{
    spot = newSpot;
    bound = preservePosition;
    if (!preservePosition && spot != nullptr) {
        desiredPosition = spot->GetPosition();
        previousTarget = spot->GetPosition();
    }
}

void CameraController::SetFrozen(bool value)
{
    if (frozen && !value && !triggers.empty()) {
        CameraTrigger* trigger = triggers.back();
        SetMode(trigger->GetMode(), trigger->GetPosition(), trigger->GetDirection(),
                trigger->GetTime(), trigger->GetPaths(), trigger->GetPathSpeed(), false,
                trigger->GetTargetMode());
        resumeFromTrigger = true;
    }
    frozen = value;
}

void CameraController::SetInstant(bool value)
{
    instant = value;
}

void CameraController::Reset()
{
    if (spot == nullptr) {
        return;
    }
    previousBoundPosition = spot->GetPosition();
    const Vec3 initialDirection = soft3d::Normalize({0, -0x6666, -0x10000});
    triggers.clear();
    inputCancelled = false;
    pathLoop = false;
    SetMode(OrbitFollow, {}, initialDirection, 0x1a8000, nullptr, 0, false, 0);
    heightDelta = soft3d::SubtractFixed(previousDesiredTarget.y, previousDesiredPosition.y);
    const Fixed verticalSquared = soft3d::FixedMultiply(heightDelta, heightDelta);
    const Fixed distanceSquared = soft3d::FixedMultiply(followDistance, followDistance);
    const Fixed horizontal = soft3d::FixedSqrt(soft3d::SubtractFixed(distanceSquared, verticalSquared));
    Vec3 lateral = soft3d::Scale(spot->GetRight(), -soft3d::fixedOne);
    lateral.y = 0;
    lateral = soft3d::Scale(lateral, horizontal);
    desiredPosition = soft3d::Add(previousBoundPosition, lateral);
    desiredPosition.y = soft3d::AddFixed(desiredPosition.y, heightDelta);
    direction = soft3d::Normalize(soft3d::Subtract(previousBoundPosition, desiredPosition));
    position = desiredPosition;
    resumeFromTrigger = true;
    inputCancelled = false;
    shakeFrames = 0;
    orbitFrames = -8;
    orbitEndFrames = -8;
    orbitRotationEnabled = false;
    orbitTargetEnabled = false;
    bound = false;
    snapMode = false;
    targetMode = 0;
    pathTime = 0;
    pathCompleted = true;
}

void CameraController::BeginMode(int32_t newMode, bool snap)
{
    if (!frozen) {
        if (!resumeFromTrigger) {
            inputCancelled = true;
        } else {
            resumeFromTrigger = false;
        }
    }
    snapMode = snap;
    previousMode = mode;
    savedDirection = direction;
    savedFollowDistance = followDistance;
    mode = newMode;
}

void CameraController::SetMode(int32_t newMode, const Vec3& newPosition, const Vec3& newDirection,
                               Fixed time, const soft3d::CameraPathCollection* newPaths,
                               Fixed newPathSpeed, bool snap, int32_t newTargetMode)
{
    switch (newMode) {
    case FixedMode: {
        BeginMode(FixedMode, snap);
        previousBoundPosition = spot != nullptr ? spot->GetPosition() : Vec3{};
        desiredPosition = newPosition;
        desiredTarget = newTargetMode == 0 ? soft3d::Add(newPosition, newDirection)
                                           : soft3d::Add(previousBoundPosition, targetOffset);
        if (!snap || instant) {
            position = desiredPosition;
            target = desiredTarget;
        }
        previousDesiredPosition = previousBoundPosition;
        previousDesiredTarget = newPosition;
        previousDirection = previousDesiredTarget;
        direction = newDirection;
        followDistance = 0xa0000;
        targetMode = newTargetMode;
        break;
    }
    case FrontFollow:
    case OrbitFollow: {
        BeginMode(newMode, snap);
        previousBoundPosition = spot != nullptr ? spot->GetPosition() : Vec3{};
        desiredPosition = soft3d::Subtract(previousBoundPosition, soft3d::Scale(newDirection, time));
        desiredTarget = soft3d::Add(previousBoundPosition, targetOffset);
        if (!snap || instant) {
            position = desiredPosition;
            target = desiredTarget;
        }
        previousDesiredPosition = previousBoundPosition;
        previousDesiredTarget = desiredPosition;
        previousDirection = previousDesiredTarget;
        direction = newDirection;
        followDistance = time;
        if (newMode == OrbitFollow) {
            heightDelta = soft3d::SubtractFixed(previousDesiredTarget.y, previousDesiredPosition.y);
        }
        break;
    }
    case OffsetFollow: {
        BeginMode(OffsetFollow, snap);
        previousBoundPosition = spot != nullptr ? spot->GetPosition() : Vec3{};
        desiredPosition = newPosition;
        desiredTarget = soft3d::Add(previousBoundPosition, targetOffset);
        if (!snap || instant) {
            position = desiredPosition;
            target = desiredTarget;
        }
        direction = newDirection;
        previousDesiredPosition = newPosition;
        previousDesiredTarget = previousDesiredPosition;
        followDistance = time;
        heightDelta = soft3d::FixedMultiply(newDirection.y, time);
        break;
    }
    case Path:
        BeginMode(Path, snap);
        paths = newPaths;
        pathSpeed = newPathSpeed;
        targetMode = newTargetMode;
        nearestPathTime = 0x8000;
        previousBoundPosition = spot != nullptr ? spot->GetPosition() : Vec3{};
        if (paths != nullptr && paths->GetCount() != 0) {
            const Fixed startDistance = soft3d::SquaredLength(soft3d::Subtract(
                paths->EvaluatePosition(0), previousBoundPosition));
            const Fixed endDistance = soft3d::SquaredLength(soft3d::Subtract(
                paths->EvaluatePosition(soft3d::fixedOne), previousBoundPosition));
            pathReverse = endDistance <= startDistance;
            if (pathSpeed == 0) {
                (void)ProjectOntoPath(true);
            } else {
                desiredPosition = paths->EvaluatePosition(0);
            }
            previousDesiredPosition = desiredPosition;
            if (targetMode == 0) {
                direction = paths->EvaluateDerivative(0);
                desiredTarget = soft3d::Add(previousDesiredPosition,
                                            soft3d::Scale(direction, 0xa0000));
            } else if (targetMode == 1) {
                direction = soft3d::Normalize(soft3d::Subtract(previousBoundPosition,
                                                                previousDesiredPosition));
                desiredTarget = soft3d::Add(previousBoundPosition, targetOffset);
            } else if (targetMode == 2) {
                direction = newDirection;
                desiredTarget = soft3d::Add(desiredPosition, direction);
            } else if (targetMode == 3) {
                direction = soft3d::Normalize(soft3d::Subtract(newDirection,
                                                                previousDesiredPosition));
                desiredTarget = soft3d::Add(newDirection, targetOffset);
                fixedTarget = newDirection;
            }
        }
        if (!snap || instant) {
            position = desiredPosition;
        }
        pathTime = 0;
        pathCompleted = false;
        break;
    case FirstPerson: {
        BeginMode(FirstPerson, snap);
        previousBoundPosition = spot != nullptr ? spot->GetPosition() : Vec3{};
        desiredPosition = soft3d::Add(previousBoundPosition, targetOffset);
        if (!snap || instant) {
            position = desiredPosition;
        }
        direction = soft3d::Normalize(newDirection);
        target = soft3d::Add(desiredPosition, soft3d::Scale(direction, 0x20000));
        desiredTarget = target;
        previousDesiredPosition = desiredPosition;
        previousDesiredTarget = previousDesiredPosition;
        firstPersonLock = false;
        freeRight = soft3d::Normalize(soft3d::Cross(worldUp, direction));
        freeUp = soft3d::Normalize(soft3d::Cross(direction, freeRight));
        firstPersonNear = 0x9999;
        firstPersonFar = 0xcccc;
        break;
    }
    case SideFollow: {
        BeginMode(SideFollow, snap);
        previousBoundPosition = spot != nullptr ? spot->GetPosition() : Vec3{};
        const Vec3 lateral = spot != nullptr ? spot->GetRight() : Vec3{soft3d::fixedOne, 0, 0};
        desiredPosition = soft3d::Add(previousBoundPosition, soft3d::Scale(lateral, time));
        target = soft3d::Add(previousBoundPosition, targetOffset);
        followDistance = time;
        direction = lateral;
        previousDesiredPosition = desiredPosition;
        previousDesiredTarget = previousDesiredPosition;
        break;
    }
    case FreeLook:
        BeginMode(FreeLook, snap);
        break;
    default:
        break;
    }
}

void CameraController::SetFirstPerson(const Vec3& newDirection, bool snap, bool lock,
                                      Fixed horizontalLimit, Fixed verticalLimit)
{
    SetMode(FirstPerson, {}, newDirection, 0, nullptr, 0, snap, 0);
    bound = snap;
    firstPersonLock = lock;
    previousDirection = previousDesiredTarget;
    freeHorizontalLimit = horizontalLimit;
    freeVerticalLimit = verticalLimit;
}

void CameraController::LookAt(const Vec3& newPosition, const Vec3& newTarget)
{
    if (camera != nullptr) {
        camera->SetLookAt(newPosition, newTarget, worldUp);
        camera->UpdateRoll();
    }
}

bool CameraController::MoveCamera(const Vec3& newPosition, const Vec3& newTarget, Fixed speed)
{
    Vec3 delta = soft3d::Subtract(position, newPosition);
    bool reachedX = false;
    bool reachedY = false;
    bool reachedZ = false;
    if (soft3d::Length(delta) > 0x28f) {
        delta = soft3d::Scale(soft3d::Normalize(delta), speed);
        MoveComponent(position.x, newPosition.x, delta.x, reachedX);
        MoveComponent(position.y, newPosition.y, delta.y, reachedY);
        MoveComponent(position.z, newPosition.z, delta.z, reachedZ);
    } else {
        position = newPosition;
        reachedX = reachedY = reachedZ = true;
    }
    delta = soft3d::Subtract(desiredTarget, newTarget);
    if (soft3d::Length(delta) > 0x28f) {
        delta = soft3d::Scale(soft3d::Normalize(delta), speed);
        bool ignored = false;
        MoveComponent(desiredTarget.x, newTarget.x, delta.x, ignored);
        MoveComponent(desiredTarget.y, newTarget.y, delta.y, ignored);
        MoveComponent(desiredTarget.z, newTarget.z, delta.z, ignored);
    } else {
        desiredTarget = newTarget;
    }
    LookAt(position, desiredTarget);
    return reachedX && reachedY && reachedZ;
}

void CameraController::Shake(int32_t frames, Fixed amplitude)
{
    shakeFrames = frames;
    shakeAmplitude = amplitude;
}

void CameraController::SetDirectFollow(bool enabled, Fixed speed)
{
    directFollow = enabled;
    if (enabled) {
        directFollowSpeed = speed;
    }
}

void CameraController::ConfigureOrbit(int32_t frames, const Vec3* center, const Vec3* newTarget,
                                      bool vertical, bool clockwise)
{
    orbitFrames = static_cast<int16_t>(frames);
    if (frames < 1) {
        orbitTargetEnabled = false;
        return;
    }
    orbitAngularVelocity = static_cast<int16_t>(0x2000 / frames);
    if (clockwise) {
        orbitAngularVelocity = static_cast<int16_t>(-orbitAngularVelocity);
    }
    if (newTarget == nullptr) {
        orbitTargetEnabled = false;
    } else {
        target = *newTarget;
        orbitTarget = *newTarget;
        orbitTargetEnabled = true;
    }
    if (center != nullptr) {
        position = soft3d::Subtract(target, *center);
        orbitVertical = center->y;
    }
    Vec3 flatPosition = position;
    flatPosition.y = 0;
    Vec3 flatTarget = target;
    flatTarget.y = 0;
    orbitRadius = Distance(flatPosition, flatTarget);
    const Vec3 delta = soft3d::Normalize(soft3d::Subtract(flatTarget, flatPosition));
    orbitAngle = soft3d::Vec3AngleXZ(delta);
    orbitFlags = vertical ? 1 : 0;
    orbitRotationEnabled = true;
    orbitInitialFrames = orbitFrames;
    orbitEndFrames = static_cast<int16_t>(orbitFrames - 8);
    const Fixed x = soft3d::FixedMultiply(soft3d::FixedCos(orbitAngle), orbitRadius);
    const Fixed z = soft3d::FixedMultiply(soft3d::FixedSin(static_cast<uint16_t>(orbitAngle)), orbitRadius);
    orbitPosition.x = soft3d::SubtractFixed(target.x, x);
    orbitPosition.y = soft3d::AddFixed(target.y, orbitVertical);
    orbitPosition.z = soft3d::SubtractFixed(target.z, z);
}

void CameraController::SetOrbitTarget(const Vec3* newTarget)
{
    if (newTarget == nullptr) {
        orbitTargetEnabled = false;
        return;
    }
    target = *newTarget;
    orbitTarget = *newTarget;
    orbitTargetEnabled = true;
}

void CameraController::SaveForTrigger()
{
    hasSavedState = true;
    saved.position = position;
    saved.target = desiredTarget;
    saved.direction = direction;
    saved.desiredPosition = desiredPosition;
    saved.followDistance = followDistance;
    saved.heightDelta = heightDelta;
    saved.pathSpeed = pathSpeed;
    saved.targetMode = targetMode;
    saved.mode = mode;
    saved.spot = spot;
    saved.paths = paths;
}

void CameraController::RestoreForTrigger(bool snap)
{
    if (hasSavedState) {
        spot = saved.spot;
        SetMode(saved.mode, saved.position,
                saved.mode == FixedMode ? saved.target : saved.direction,
                saved.followDistance, paths, saved.pathSpeed, snap,
                saved.mode == FixedMode || saved.mode == Path ? 0 : saved.targetMode);
        if (saved.mode == Path) {
            paths = saved.paths;
        }
    }
    heightDelta = saved.heightDelta;
    targetMode = saved.targetMode;
    pathSpeed = saved.pathSpeed;
}

void CameraController::AddTrigger(CameraTrigger* trigger)
{
    triggers.push_back(trigger);
}

void CameraController::RemoveTrigger(CameraTrigger* trigger)
{
    const auto found = std::find(triggers.begin(), triggers.end(), trigger);
    if (found != triggers.end()) {
        triggers.erase(found);
    }
}

void CameraController::UpdateTriggers()
{
    const std::size_t count = triggers.size();
    for (std::size_t index = 0; index < count; ++index) {
        triggers[index]->Update();
    }
}

uint32_t CameraController::Random()
{
    uint32_t* state = randomState != nullptr ? randomState : &fallbackRandomState;
    *state = *state * 0x343fdU + 0x269ec3U;
    return (*state >> 16) & 0x7fffU;
}

int32_t CameraController::RandomShakeComponent()
{
    return static_cast<int32_t>(Random() % 192U / 64U) - 1;
}

bool CameraController::MovementInput() const
{
    if (controls == nullptr || controlInput == nullptr) {
        return false;
    }
    return controls->ActionDown(*controlInput, 1, false) || controls->ActionDown(*controlInput, 2, false) ||
           controls->ActionDown(*controlInput, 3, false) || controls->ActionDown(*controlInput, 4, false) ||
           controls->ActionClick(*controlInput, 1, false) || controls->ActionClick(*controlInput, 2, false) ||
           controls->ActionClick(*controlInput, 3, false) || controls->ActionClick(*controlInput, 4, false);
}


soft3d::Fixed CameraController::ProjectOntoPath(bool initializing)
{
    if (paths == nullptr || paths->GetCount() == 0) {
        desiredPosition = {};
        return 0;
    }
    Vec3 start = paths->EvaluatePosition(0);
    Vec3 end = paths->EvaluatePosition(soft3d::fixedOne);
    Vec3 pathDirection = soft3d::Subtract(end, start);
    const Fixed pathLength = soft3d::Length(pathDirection);
    pathDirection = soft3d::Normalize(pathDirection);
    const Fixed inverseLength = pathLength == 0 ? 0 : 0x40000000 / (pathLength >> 2);
    const Vec3 lookTarget = soft3d::Add(spot != nullptr ? spot->GetPosition() : Vec3{}, targetOffset);
    desiredTarget = lookTarget;
    const Vec3 relative = soft3d::Subtract(lookTarget, start);
    const Fixed projection = soft3d::Dot(relative, pathDirection);
    Fixed selectedTime = 0;
    if (projection < 0) {
        desiredPosition = start;
    } else if (projection > pathLength) {
        desiredPosition = end;
        selectedTime = soft3d::fixedOne;
    } else {
        selectedTime = soft3d::FixedMultiply(projection, inverseLength);
        desiredPosition = paths->EvaluatePosition(selectedTime);
    }
    if (followDistance == 0) {
        return selectedTime;
    }

    const Fixed along = soft3d::Dot(soft3d::Subtract(lookTarget, start), pathDirection);
    const Vec3 perpendicular = soft3d::Subtract(
        soft3d::Subtract(lookTarget, start), soft3d::Scale(pathDirection, along));
    start = soft3d::Add(start, perpendicular);
    end = soft3d::Add(end, perpendicular);
    const Vec3 fromSpot = soft3d::Subtract(start, spot != nullptr ? spot->GetPosition() : Vec3{});
    const Fixed a = soft3d::SquaredLength(fromSpot);
    const Fixed b = soft3d::ShiftLeftFixed(soft3d::Dot(pathDirection, fromSpot), 1);
    const Fixed c = soft3d::SubtractFixed(a, soft3d::FixedMultiply(followDistance, followDistance));
    Fixed discriminant = soft3d::ShiftLeftFixed(
        soft3d::SubtractFixed(soft3d::FixedMultiply(b, b), soft3d::FixedMultiply(0x40000, c)), 2);
    Fixed initialTime = selectedTime;
    selectedTime = 0;
    if (discriminant <= 0) {
        if (!initializing) {
            discriminant = soft3d::fixedOne;
        } else {
            desiredPosition = paths->EvaluatePosition(initialTime);
            return initialTime;
        }
    }
    if (discriminant > 0) {
        const Fixed root = soft3d::FixedSqrt(discriminant);
        const Fixed negativeB = soft3d::NegateFixed(b);
        const Fixed firstRoot = soft3d::SubtractFixed(negativeB, root) >> 1;
        const Fixed secondRoot = soft3d::AddFixed(negativeB, root) >> 1;
        const Vec3 firstPoint = soft3d::Add(start, soft3d::Scale(pathDirection, firstRoot));
        const Vec3 secondPoint = soft3d::Add(start, soft3d::Scale(pathDirection, secondRoot));
        Fixed firstTime = soft3d::FixedMultiply(
            soft3d::Dot(soft3d::Subtract(firstPoint, start), pathDirection), inverseLength);
        Fixed secondTime = soft3d::FixedMultiply(
            soft3d::Dot(soft3d::Subtract(secondPoint, start), pathDirection), inverseLength);
        firstTime = std::clamp(firstTime, 0, soft3d::fixedOne);
        secondTime = std::clamp(secondTime, 0, soft3d::fixedOne);
        if (!pathReverse) {
            selectedTime = std::min(firstTime, secondTime);
        } else {
            selectedTime = soft3d::AbsFixed(soft3d::SubtractFixed(firstTime, nearestPathTime)) <
                    soft3d::AbsFixed(soft3d::SubtractFixed(secondTime, nearestPathTime))
                ? firstTime : secondTime;
        }
        if (!initializing && selectedTime != nearestPathTime) {
            const Fixed difference = soft3d::SubtractFixed(selectedTime, nearestPathTime);
            const Fixed magnitude = soft3d::AbsFixed(difference);
            selectedTime = soft3d::AddFixed(nearestPathTime,
                                             soft3d::FixedMultiply(difference, magnitude));
            if (soft3d::SquaredLength(soft3d::Subtract(paths->EvaluatePosition(selectedTime),
                    paths->EvaluatePosition(nearestPathTime))) < 0x41) {
                selectedTime = nearestPathTime;
            }
        }
        nearestPathTime = selectedTime;
    }
    desiredPosition = paths->EvaluatePosition(selectedTime);
    return selectedTime;
}

void CameraController::UpdatePath(bool)
{
    if (paths == nullptr || paths->GetCount() == 0) {
        pathCompleted = true;
        return;
    }
    if (pathSpeed == 0) {
        ProjectOntoPath(false);
        if (followDistance == 0) {
            position = desiredPosition;
        }
    } else {
        desiredPosition = paths->EvaluatePosition(pathTime);
        const Fixed step = soft3d::FixedDivide(
            soft3d::fixedOne, soft3d::FixedMultiply(0x100000, pathSpeed));
        pathTime = soft3d::AddFixed(pathTime, step);
        if (pathTime > soft3d::fixedOne) {
            pathCompleted = true;
            pathTime = pathLoop ? 0 : soft3d::fixedOne;
        }
    }
    if (targetMode == 0) {
        desiredTarget = soft3d::Add(position,
            soft3d::Scale(paths->EvaluateDerivative(pathTime), 0xa0000));
    } else if (targetMode == 1) {
        desiredTarget = soft3d::Add(spot != nullptr ? spot->GetPosition() : Vec3{}, targetOffset);
    } else if (targetMode == 2) {
        desiredTarget = soft3d::Add(position, direction);
    } else if (targetMode == 3) {
        desiredTarget = fixedTarget;
    }
    if (spot != nullptr) {
        const Vec3 side = soft3d::Add(soft3d::Subtract(position, previousBoundPosition), targetOffset);
        if (soft3d::Dot(side, spot->GetRight()) > 0) {
            inputCancelled = true;
        }
    }
    if (MovementInput()) {
        inputCancelled = false;
    }
}

void CameraController::UpdateFixed(bool)
{
    desiredPosition = previousDesiredTarget;
    if (targetMode == 1 && spot != nullptr) {
        desiredTarget = soft3d::Add(spot->GetPosition(), targetOffset);
    } else if (targetMode == 0) {
        desiredTarget = soft3d::Add(previousDesiredTarget, direction);
    }
    if (MovementInput()) {
        inputCancelled = false;
    }
}

void CameraController::UpdateFrontFollow(bool)
{
    desiredPosition = soft3d::Subtract(previousBoundPosition,
                                       soft3d::Scale(direction, followDistance));
    previousDesiredTarget = desiredPosition;
    desiredTarget = soft3d::Add(previousBoundPosition, targetOffset);
    if (MovementInput()) {
        inputCancelled = false;
    }
    ClipCamera();
}

void CameraController::UpdateOrbitFollow(bool immediate)
{
    Vec3 offset = soft3d::Subtract(position, target);
    if (!Near(previousBoundPosition, previousDesiredPosition) || immediate) {
        const Fixed horizontal = soft3d::FixedSqrt(soft3d::SubtractFixed(
            soft3d::FixedMultiply(followDistance, followDistance),
            soft3d::FixedMultiply(heightDelta, heightDelta)));
        offset.y = 0;
        if (soft3d::AbsFixed(offset.x) > 0x1999 || soft3d::AbsFixed(offset.z) > 0x1999) {
            offset = soft3d::Normalize(offset);
        }
        desiredPosition = soft3d::Add(target, soft3d::Scale(offset, horizontal));
        desiredPosition.y = soft3d::AddFixed(desiredPosition.y, heightDelta);
        previousDesiredTarget = desiredPosition;
        direction = soft3d::Normalize(soft3d::Subtract(previousBoundPosition, desiredPosition));
    }
    desiredTarget = previousBoundPosition;
    ClipCamera();
}

void CameraController::UpdateOffsetFollow(bool)
{
    desiredPosition = previousDesiredPosition;
    desiredTarget = previousBoundPosition;
    ClipCamera();
}

void CameraController::UpdateSideFollow(bool)
{
    const Vec3 lateral = spot != nullptr ? spot->GetRight() : Vec3{soft3d::fixedOne, 0, 0};
    desiredPosition = soft3d::Add(soft3d::Subtract(previousBoundPosition,
        soft3d::Scale(lateral, followDistance)), targetOffset);
    if (MovementInput()) {
        inputCancelled = false;
    }
    ClipCamera();
}

void CameraController::UpdateFirstPerson(bool)
{
    Vec3 look = soft3d::Subtract(target, position);
    Vec3 lateral = soft3d::Normalize(soft3d::Cross(worldUp, look));
    uint16_t yaw = 0;
    uint16_t pitch = 0;
    if (controls != nullptr && controlInput != nullptr) {
        if (controls->ActionDown(*controlInput, 3, false)) yaw = static_cast<uint16_t>(yaw - 0x51);
        if (controls->ActionDown(*controlInput, 4, false)) yaw = static_cast<uint16_t>(yaw + 0x51);
        if (controls->ActionDown(*controlInput, 1, false)) pitch = static_cast<uint16_t>(pitch - 0x51);
        if (controls->ActionDown(*controlInput, 2, false)) pitch = static_cast<uint16_t>(pitch + 0x51);
    }
    Matrix4 rotation = AxisAngleMatrix(worldUp, yaw);
    rotation = soft3d::Multiply(rotation, AxisAngleMatrix(lateral, pitch));
    look = soft3d::Transform(rotation, look);
    const Vec3 normalized = soft3d::Normalize(look);
    if (soft3d::AbsFixed(normalized.y) <= 0xe666) {
        target = soft3d::Add(position, look);
        desiredTarget = soft3d::Add(previousBoundPosition, targetOffset);
        desiredPosition = target;
        ClipCamera();
        Vec3 forward = soft3d::Subtract(position, target);
        forward.y = 0;
        forward = soft3d::Normalize(forward);
        const Vec3 newRight = soft3d::Cross(forward, worldUp);
        if (spot != nullptr) {
            spot->SetRight(newRight);
            spot->SetUp(worldUp);
            spot->SetForward(forward);
        }
    }
}

void CameraController::UpdateFreeLook(bool)
{
    Vec3 look = soft3d::Subtract(target, position);
    Vec3 lateral = soft3d::Normalize(soft3d::Cross(worldUp, soft3d::Subtract(position, target)));
    if (freeLookLocked) {
        position = previousBoundPosition;
    }
    uint16_t yaw = 0;
    uint16_t pitch = 0;
    if (controls != nullptr && controlInput != nullptr) {
        if (controls->ActionDown(*controlInput, 3, false)) yaw = static_cast<uint16_t>(yaw + 0x51);
        if (controls->ActionDown(*controlInput, 4, false)) yaw = static_cast<uint16_t>(yaw - 0x51);
        if (!freeLookLocked) {
            if (controls->ActionDown(*controlInput, 1, false)) pitch = static_cast<uint16_t>(pitch + 0x36);
            if (controls->ActionDown(*controlInput, 2, false)) pitch = static_cast<uint16_t>(pitch - 0x36);
        }
    }
    if (pitch != 0) {
        look = RotateAroundAxis(look, lateral, pitch);
    }
    if (yaw != 0) {
        look = RotateAroundAxis(look, worldUp, yaw);
    }
    const Vec3 normalized = soft3d::Normalize(look);
    if (!freeLookLocked && controls != nullptr && controlInput != nullptr) {
        if (controls->ActionDown(*controlInput, 3, false) ||
            controls->ActionDown(*controlInput, 4, false)) {
            Vec3 flat = normalized;
            flat.y = 0;
            flat = soft3d::Normalize(flat);
            const Fixed dot = soft3d::Dot(flat, freeReference);
            if ((controls->ActionDown(*controlInput, 3, false) &&
                 dot < soft3d::NegateFixed(freeHorizontalLimit)) ||
                (controls->ActionDown(*controlInput, 4, false) && dot > freeHorizontalLimit)) {
                return;
            }
        }
        if (controls->ActionDown(*controlInput, 1, false) ||
            controls->ActionDown(*controlInput, 2, false)) {
            const Fixed dot = soft3d::Dot(normalized, worldUp);
            if ((controls->ActionDown(*controlInput, 2, false) &&
                 dot < soft3d::NegateFixed(freeVerticalLimit)) ||
                (controls->ActionDown(*controlInput, 1, false) && dot > freeVerticalLimit)) {
                return;
            }
        }
    }
    target = soft3d::Add(position, normalized);
    desiredTarget = target;
}

void CameraController::ClipCamera()
{
    if (!collisionEnabled || bsp == nullptr) {
        return;
    }
    if (mode == OrbitFollow && Near(desiredPosition, previousDesiredPosition)) {
        return;
    }
    const soft3d::ObjectBounds bounds{{-soft3d::fixedOne, -soft3d::fixedOne, -soft3d::fixedOne},
                                      {soft3d::fixedOne, soft3d::fixedOne, soft3d::fixedOne}};
    soft3d::BspTraceResult forward = bsp->Trace(desiredTarget, desiredPosition, bounds);
    if (!forward.startSolid) {
        desiredPosition = forward.endPosition;
    } else {
        soft3d::BspTraceResult backward = bsp->Trace(desiredPosition, desiredTarget, bounds);
        const Fixed normals = soft3d::AbsFixed(soft3d::Dot(forward.planeNormal, backward.planeNormal));
        if (normals > 0xe666) {
            desiredPosition = forward.endPosition;
            if (soft3d::AddFixed(soft3d::Dot(forward.planeNormal, position),
                                 forward.planeDistance) < 0) {
                position = forward.endPosition;
            }
        } else {
            Vec3 normal = soft3d::Add(forward.planeNormal, backward.planeNormal);
            if (!Near(normal, {})) {
                normal = soft3d::Scale(soft3d::Normalize(normal), 0x80000);
            }
            Vec3 candidate = soft3d::Add(forward.endPosition, normal);
            backward = bsp->Trace(desiredPosition, candidate, bounds);
            if (!backward.startSolid) {
                backward = bsp->Trace(desiredTarget, candidate, bounds);
                if (!backward.startSolid) {
                    forward.endPosition = candidate;
                } else {
                    position = forward.endPosition;
                }
            } else {
                candidate = backward.endPosition;
                backward = bsp->Trace(desiredTarget, candidate, bounds);
                if (!backward.startSolid) {
                    forward.endPosition = candidate;
                } else {
                    position = forward.endPosition;
                }
            }
            desiredPosition = forward.endPosition;
        }
    }
    const Vec3 delta = soft3d::Subtract(desiredPosition, desiredTarget);
    const Fixed length = soft3d::Length(delta);
    if (length < followDistance) {
        const Fixed vertical = soft3d::AbsFixed(soft3d::SubtractFixed(desiredPosition.y, desiredTarget.y));
        const Fixed horizontal = soft3d::FixedSqrt(soft3d::SubtractFixed(
            soft3d::FixedMultiply(length, length), soft3d::FixedMultiply(vertical, vertical)));
        const Fixed remainder = soft3d::FixedSqrt(soft3d::SubtractFixed(
            soft3d::FixedMultiply(followDistance, followDistance),
            soft3d::FixedMultiply(horizontal, horizontal)));
        Vec3 candidate = soft3d::Add(desiredTarget, soft3d::Scale(soft3d::Normalize(delta), horizontal));
        candidate.y = soft3d::AddFixed(candidate.y, remainder);
        const Vec3 restored = desiredPosition;
        candidate.x = restored.x;
        candidate.z = restored.z;
        const soft3d::BspTraceResult verticalTrace = bsp->Trace(desiredPosition, candidate, bounds);
        desiredPosition = verticalTrace.startSolid ? verticalTrace.endPosition : candidate;
    }
}

void CameraController::UpdateSmoothing()
{
    positionVelocity = soft3d::Scale(soft3d::Subtract(desiredPosition, position), 0x1999);
    position = soft3d::Add(position, positionVelocity);
    position = soft3d::Add(position, soft3d::Scale(targetVelocity,
        soft3d::FixedMultiply(0x20000, 0x1999)));
    position = soft3d::Add(position, acceleration);
    target = desiredTarget;
}

void CameraController::UpdateOrientation()
{
    forward = soft3d::Normalize(soft3d::Subtract(target, position));
    up = worldUp;
    const Fixed alignment = soft3d::AbsFixed(soft3d::Dot(up, forward));
    if (alignment > 0xfae0) {
        right = soft3d::Subtract(right, soft3d::Scale(forward, soft3d::Dot(right, forward)));
        right = soft3d::Normalize(right);
        up = soft3d::Normalize(soft3d::Cross(forward, right));
        right = soft3d::Normalize(soft3d::Cross(up, forward));
    } else {
        right = soft3d::Cross(up, forward);
        if (soft3d::Length(right) < 0x8000) {
            right = soft3d::Scale(right, 0x640000);
        }
        right = soft3d::Normalize(right);
        up = soft3d::Normalize(soft3d::Cross(forward, right));
        right = soft3d::Normalize(soft3d::Cross(up, forward));
    }
}

void CameraController::UpdateOrbit()
{
    if (!orbitTargetEnabled) {
        orbitTarget = target;
    } else {
        target = orbitTarget;
    }
    if (shakeFrames != 0) {
        target.x = soft3d::AddFixed(target.x,
            soft3d::FixedMultiply(RandomShakeComponent() * soft3d::fixedOne, shakeAmplitude));
        target.y = soft3d::AddFixed(target.y,
            soft3d::FixedMultiply(RandomShakeComponent() * soft3d::fixedOne, shakeAmplitude));
        target.z = soft3d::AddFixed(target.z,
            soft3d::FixedMultiply(RandomShakeComponent() * soft3d::fixedOne, shakeAmplitude));
        --shakeFrames;
    }
    if (orbitFrames < 1) {
        if (orbitFrames >= -8) {
            const Fixed alpha = soft3d::FixedDivide((orbitFrames + 8) * soft3d::fixedOne, 0x80000);
            position = Lerp(position, orbitPosition, alpha);
            target = Lerp(target, orbitTarget, alpha);
            --orbitFrames;
        }
        return;
    }
    if (orbitEndFrames < orbitFrames) {
        const Fixed alpha = soft3d::FixedDivide((orbitEndFrames - orbitFrames) * soft3d::fixedOne,
                                                0x80000);
        position = Lerp(position, orbitPosition, alpha);
        target = Lerp(target, orbitTarget, alpha);
        --orbitFrames;
        return;
    }
    if (orbitRotationEnabled) {
        orbitAngle = static_cast<int16_t>(orbitAngle + orbitAngularVelocity);
        if (orbitAngle < 1) orbitAngle = static_cast<int16_t>(orbitAngle + 0x2000);
        if (orbitAngle > 0x1fff) orbitAngle = static_cast<int16_t>(orbitAngle - 0x2000);
    }
    position.x = soft3d::SubtractFixed(target.x,
        soft3d::FixedMultiply(soft3d::FixedCos(orbitAngle), orbitRadius));
    position.z = soft3d::SubtractFixed(target.z,
        soft3d::FixedMultiply(soft3d::FixedSin(static_cast<uint16_t>(orbitAngle)), orbitRadius));
    position.y = soft3d::AddFixed(target.y, orbitVertical);
    --orbitFrames;
    if ((orbitFlags & 1) != 0 && orbitFrames < 1) {
        orbitFrames = orbitInitialFrames;
        orbitEndFrames = orbitFrames;
    }
    orbitPosition = position;
}

void CameraController::Update(const Vec3* targetOverride, bool immediate)
{
    if (spot == nullptr) {
        return;
    }
    previousBoundPosition = targetOverride != nullptr ? *targetOverride : spot->GetPosition();
    if (((mode == Path || mode == FixedMode) && targetMode != 0) ||
        mode == FrontFollow || mode == OrbitFollow || mode == OffsetFollow || mode == SideFollow) {
        desiredTarget = soft3d::Add(previousBoundPosition, targetOffset);
    }
    if (!snapMode) {
        target = desiredTarget;
    }
    if (MovementInput()) {
        inputCancelled = false;
    }
    if (!snapMode || instant) {
        switch (mode) {
        case FixedMode: UpdateFixed(immediate); break;
        case FrontFollow: UpdateFrontFollow(immediate); break;
        case OrbitFollow: UpdateOrbitFollow(immediate); break;
        case OffsetFollow: UpdateOffsetFollow(immediate); break;
        case Path: UpdatePath(immediate); break;
        case FirstPerson: UpdateFirstPerson(immediate); break;
        case SideFollow: UpdateSideFollow(immediate); break;
        case FreeLook: UpdateFreeLook(immediate); break;
        default: break;
        }
        if (mode != FirstPerson) {
            if (mode == FrontFollow) {
                const Vec3 delta = soft3d::Subtract(desiredPosition, position);
                const Vec3 projected = soft3d::Scale(right, soft3d::Dot(delta, right));
                position = soft3d::Add(position, soft3d::Scale(projected, 0x1999));
            }
            UpdateSmoothing();
        }
    } else {
        switch (mode) {
        case FixedMode: UpdateFixed(immediate); break;
        case FrontFollow: UpdateFrontFollow(immediate); break;
        case OrbitFollow: UpdateOrbitFollow(immediate); break;
        case OffsetFollow: UpdateOffsetFollow(immediate); break;
        case Path: UpdatePath(immediate); break;
        case FirstPerson: UpdateFirstPerson(immediate); break;
        case SideFollow: UpdateSideFollow(immediate); break;
        case FreeLook: UpdateFreeLook(immediate); break;
        default: break;
        }
        const Vec3 positionDelta = soft3d::Subtract(desiredPosition, position);
        const Vec3 targetDelta = soft3d::Subtract(desiredTarget, target);
        Vec3 targetStep;
        if (directFollow) {
            const Fixed distance = soft3d::Length(positionDelta);
            if (distance > 0x28f) {
                const Vec3 normalized = soft3d::Normalize(positionDelta);
                targetStep = soft3d::Scale(targetDelta,
                    soft3d::FixedDivide(directFollowSpeed, distance));
                positionVelocity = soft3d::Scale(normalized,
                    std::min(directFollowSpeed, distance));
            } else {
                positionVelocity = positionDelta;
                targetStep = targetDelta;
            }
            position = soft3d::Add(position, positionVelocity);
            target = soft3d::Add(target, targetStep);
        } else {
            positionVelocity = soft3d::Scale(positionDelta, 0x1999);
            const Vec3 desiredForward = soft3d::Normalize(soft3d::Subtract(desiredTarget, position));
            const Vec3 currentForward = soft3d::Normalize(soft3d::Subtract(target, position));
            targetStep = soft3d::Scale(soft3d::Subtract(desiredForward, currentForward), 0x2666);
            position = soft3d::Add(position, positionVelocity);
            target = soft3d::Add(soft3d::Add(position, currentForward), targetStep);
        }
        if (soft3d::AbsFixed(positionVelocity.x) <= 0x20 &&
            soft3d::AbsFixed(positionVelocity.y) <= 0x20 &&
            soft3d::AbsFixed(positionVelocity.z) <= 0x20 &&
            soft3d::AbsFixed(targetStep.x) <= 0x20 &&
            soft3d::AbsFixed(targetStep.y) <= 0x20 &&
            soft3d::AbsFixed(targetStep.z) <= 0x20) {
            snapMode = false;
        }
    }
    targetVelocity = {};
    acceleration = {};
    UpdateOrientation();
    UpdateOrbit();
    if (camera != nullptr) {
        camera->SetLookAt(position, target, up);
        camera->UpdateRoll();
    }
    previousPosition = position;
    previousDirection = desiredTarget;
    previousTarget = target;
    previousDesiredPosition = previousBoundPosition;
}

} // namespace game
