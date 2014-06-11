# Conjoint

```
gyp --depth=. --suffix=-gyp
xcodebuild -ARCHS="x86_64" -project conjoint-gyp.xcodeproj
./build/Default/conjoint examples/allFeatures.cj
```
