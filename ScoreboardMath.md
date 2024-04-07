## Usage in your own code
You can use the calculations by importing ScoreboardPosition.h to your own project and  
Replace  
```
float tier_X = -SCOREBOARD_LEFT - IMAGE_WIDTH * IMAGE_SCALE;
```  
with  
```
float tier_X = -SCOREBOARD_LEFT;
```
  
**Return values**  
```blueLeaderPos``` and ```orangeLeaderPos``` will point to the top left corner of the blue and orange parts respectively.  
You can add ```playerSeparation``` to the Y coordinates to find the next player's top left corner and add again for the next and so on.  
The ```scale``` variable will contain the scaling factor of the scoreboard compared to a 1920*1080 display with Interface Scale and Display Scale set to 100%.  
Multiply image scales, and any additions/subtractions to coordinates around the scoreboard by this value to make it compatible with any screen size and scale settings
  
**Parameters**  
```canvas size```: pass ```gameWrapper->GetScreenSize()``` to it.  
```uiScale```: pass ```gameWrapper->GetInterfaceScale() * gameWrapper->GetDisplayScale()``` to it.  
```mutators```: pass true when mutators are active (assumes mutator size of the tournament 11 minute mutator)  
```numBlues, numOranges```: number of blue and orange players on the scoreboard  
```isReplaying```: For general purposes set it to false, if you're putting something to the left of the scoreboard set it to true when in instant replay and the coordinates will be moved to get out of the way of the green check marks.  
