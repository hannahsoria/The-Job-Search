// Hannah Soria
// The Job Search
// Design Studio 2026

#include <SPI.h>
#include "Adafruit_RA8875.h"
#include "ready.h"
#include "unethical.h"
#include "buddy.h"
#include "professional.h"
#include "distracted.h"
#include "pressured.h"
#include "final.h"
#include "lose.h"
#include "win.h"
#include "prep.h"
#include <string.h>

//-------------------------------SETUP---------------------------------------------

// RA8875 display
#define RA8875_INT 27
#define RA8875_CS 21
#define RA8875_RESET 22
Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);

// Buttons
#define btn1 32
#define btn2 33
#define btn3 25
#define btn4 26
#define btn5 13       // HOME / RESTART 
#define btn_enter 14

// game states
enum GameState {
  TITLE, LEARN_MORE, SOURCES, CHOOSE_NAME, CHOOSE_AGE, CHOOSE_SCHOOL,
  SHOP_INTRO, SHOP, INSTRUCTIONS, READY, INTERVIEW_INTRO, INTERVIEW, CROSSING_INTRO, CROSSING, EVENT_SCREEN,
  WIN_SCREEN, LOSE_SCREEN, STATS_SCREEN, STATS_SCREEN2, END
};

// start on the title
GameState state = TITLE;

// "crossings"
enum CrossingType { FOLLOW_UP_EMAIL, FINAL_PREP };

// Interviewer types are intentionally hidden from the player.
enum InterviewerType {
  UNETHICAL,
  BUDDY,
  PROFESSIONAL,
  DISTRACTED,
  PRESSURING,
  INTERVIEWER_TYPE_COUNT
};

// Candidate setup
const char *candidateName = "Not selected";
int nameChoice = 0;       // 1 Brad, 2 Ebony, 3 Lamar, 4 Kirsten
int age = 0;              // 20, 30, 40, or 50
int schoolChoice = 0;     // 1 Ivy, 2 state, 3 community

// Candidate and interview scoring
const int MAX_HEALTH = 100;
int health = 0;                 // determines rejection or not 
int practice = 0;
int childcare = 0;
int latte = 0;
int linkedin = 0;
float money = 800.00;
int preparationValue = 0;

int interviewNumber = 1;
int interviewStep = 0;
int currentInterviewer = 0;
bool interviewerUsed[5] = {false, false, false, false, false};
int interviewerHistory[3] = {-1, -1, -1};
int currentCrossing = 0;
int lastHealthChange = 0;

const char *crossingOutcome = "";
char lineBuffer[96];

//-------------------------------BTN AND SCREEN CMDS---------------------------------

// ensure no double press
bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW) {
    while (digitalRead(pin) == LOW) delay(10);
    delay(35);
    return true;
  }
  return false;
}

// clear entire screen
void clearScreen() {
  tft.graphicsMode();
  tft.fillScreen(RA8875_BLACK);
  tft.textMode();
  tft.textEnlarge(0);
  tft.textColor(RA8875_GREEN, RA8875_BLACK);
}

// write body text
void writeText(int x, int y, const char *text) {
  const char *lineStart = text;
  int lineNumber = 0;

  while (true) {
    const char *lineEnd = strchr(lineStart, '\n');

    int len = lineEnd ? lineEnd - lineStart : strlen(lineStart);

    char line[128];

    if (len > 127) len = 127;

    strncpy(line, lineStart, len);
    line[len] = '\0';

    // Remove carriage returns if present
    if (len > 0 && line[len - 1] == '\r') {
      line[len - 1] = '\0';
    }

    // Draw each line separately
    tft.textSetCursor(x, y + (lineNumber * 20));
    tft.textWrite(line);

    if (!lineEnd) break;

    lineStart = lineEnd + 1;
    lineNumber++;
  }
}

// write body text
void writeText(int x, int y, const __FlashStringHelper *text) {
  char buffer[96];
  strcpy_P(buffer, (PGM_P)text);
  writeText(x, y, buffer);
}

// write a big title in top left
void writeTitle(const char *text) {
  tft.textEnlarge(1);
  tft.textSetCursor(20, 20);
  tft.textWrite(text);
  tft.textEnlarge(0);
}

// write a big title in top left
void writeTitle(const __FlashStringHelper *text) {
  char buffer[96];
  strcpy_P(buffer, (PGM_P)text);
  writeTitle(buffer);
}

// pressing back resets the game
// appears on every screen
void showHomeLabel() {
  tft.textSetCursor(580, 20);
  tft.textWrite("Press [Back] to reset game");
}

// resert screen
void finishScreen() {
  showHomeLabel();
}

//-------------------------------INTERVIEW CONTENT---------------------------------

// interview questions from 5 different interviewer types
const char* interviewQuestions[5][3] = {
  // unethical
  {
    "Tell me about yourself?",
    "Where are you from originally?",
    "Do you plan to start a family soon?"
  },

  // Buddy
  {
    "Tell me about yourself?",
    "What do you like to do in your free time?",
    "On Fridays, we all order lunch. What's your favorite local spot?"
  },

  // Professional
  {
    "Tell me about yourself?",
    "What makes you qualified for this role?",
    "Do you have any questions for me?"
  },

  // Distracted
  {
    "Tell me about yourself?",
    "Can you remind me what position you are applying for?",
    "Actually I am getting a call, I need to take this."
  },

  // Pressuring
  {
    "Tell me about yourself?",
    "We need someone willing to put in the extra hours. Is this an issue?",
    "This job is is a high stakes environment. Can you handle that?"
  }
};

// corresponding possivle answers for each interviewer
// vary on impact form good, neutral (kinda negative), negative
const char* interviewAnswers[5][3][3] = {
 // UNETHICAL INTERVIEWER
 // Q1: 1 - middle, 2 - negative, 3 - good
 // Q2: 1 - middle, 2 - negative, 3 - good
 // Q3: 1 - negative, 2 - negative, 3 - good
 {
   { "Recite your entire resume chronologically.", "Tell them about your hobbies and interests.", "State your current title and responsibilities." },
   { "Deep dive into how awesome your childhood home is.", "Stay professional and talk about where you live now.", "Tell them where you were born and where your parents are from." },
   { "Tell them that is too personal of a question.", "Share with them that you are actually going to have a child soon!", "Assure them you'll meet whatever requirements the company needs." }
 },
 // BUDDY INTERVIEWER
 // Q1: 1 - negative, 2 - good, 3 - middle
 // Q2: 1 - good, 2 - middle, 3 - bad
 // Q3: 1 - good, 2 - middle, 3 - bad
 {
   { "Share your life story, starting with where you were born.", "Explain why you're more qualified than everyone else applying.", "Talk about your recent jobs and your personal hobbies." },
   { "Share that you are an avid golfer; ask if they want to join for a round at the country\nclub you belong to.", "Share that you love to go on hikes with your dog and play video games in your free time.", "Tell them you love to network and dedicate personal time to professional development." },
   { "Share that your open to anything as long as your with people.", "Tell them where your favorite coffee spot is.", "Say you save money by always making food at home." }
 },
 // PROFESSIONAL INTERVIEWER
 // Q1: 1 - negative, 2 - middle, 3 - good
 // Q2: 1 - middle, 2 - good, 3 - negative
 // Q3: 1 - negative, 2 - middle, 3 - good
 {
   { "Share how you\'ll work hard to fill any gaps.", "Talk about where you went to college and what clubs you were in.", "Give a thoughtful story about your professional journey." },
   { "List your skills without explaining how you've used them.", "Highlight your relevant skills and experience, with examples of past achievements.", "Promise to work weekends without expecting extra pay."
 },
   { "Say \"Why did the last person leave this role?\"", "Ask if the company requires a drug test.", "Ask what the biggest challenges are on the team right now." }
 },
 // DISTRACTED INTERVIEWER
 // Q1: 1 - middle, 2 - middle, 3 - middle
 // Q2: 1 - good, 2 - middle, 3 - negative
 // Q3: 1 - bad, 2 - good, 3 - good
 {
   { "Tell them about your success at yout last job.", "Tell them about how your college sport experience makes you resilient.", "Ask what they want to know specifically." },
   { "Make a joke that you have a bad memory too.", "State the title for them and ask if you're in the right place.", "Blank on the title yourself and say, \"It\'s the one you\'re interviewing me for\"." },
   { "Stand up for yourself! Tell them that you have already spent enough time waiting.", "Politely tell them to take their time and that you are happy to wait.", "Nod and scroll on Instagram reels when they answer the call." }
 },
 // PRESSURING INTERVIEWER
 // Q1: 1 - middle, 2 - good, 3: bad
 // Q2: 1 - good, 2 - bad, 3 - middle
 // Q3: 1 - good, 2 - middle, 3 - bad
 {
   { "Recite your resume in chronological order.", "Make up achievements to sound impressive.", "Ramble about your personal life and hobbies." },
   { "Say you'll do anything for this job.", "Say that you\'ll only stay late if you\'re in the mood.", "Bring up overtime pay before showing any willingness." },
   { "Tell them your last job overworked you but you stayed because you hate to quit.", "Say that you feel motivated by a bit of stress.", "Describe a time where you shut down, cried, and snapped at a former co-worker\nwhen you were overwhelmed." }
 }
};

// Tte preferred answer for each interviewer type and question
// answer quality: WORST = 0, NEUTRAL = 1, BEST = 2
enum AnswerQuality { WORST = 0, NEUTRAL = 1, BEST = 2 };

const int answerQuality[INTERVIEWER_TYPE_COUNT][3][3] = {

  // UNETHICAL INTERVIEWER
  {
    { NEUTRAL, WORST, BEST },    // Q1: middle, negative, good
    { NEUTRAL, WORST, BEST },    // Q2: middle, negative, good
    { WORST, WORST, BEST }       // Q3: negative, negative, good
  },

  // BUDDY INTERVIEWER
  {
    { WORST, NEUTRAL, BEST },    // Q1: negative, good, middle
    { BEST, NEUTRAL, WORST },    // Q2: good, middle, bad
    { BEST, NEUTRAL, WORST }     // Q3: good, middle, bad
  },

  // PROFESSIONAL INTERVIEWER
  {
    { WORST, NEUTRAL, BEST },    // Q1: worst, middle, good
    { NEUTRAL, BEST, WORST },    // Q2: middle, good, negative
    { WORST, NEUTRAL, BEST }     // Q3: negative, middle, good
  },

  // DISTRACTED INTERVIEWER
  {
    { NEUTRAL, NEUTRAL, NEUTRAL },   // Q1: middle, middle, middle
    { BEST, NEUTRAL, WORST },        // Q2: negative, good, negative
    { WORST, BEST, BEST }            // Q3: bad, good, good
  },

  // PRESSURING INTERVIEWER
  {
    { NEUTRAL, BEST, WORST },   // Q1: middle, good, bad
    { BEST, WORST, NEUTRAL },   // Q2: good, bad, middle
    { BEST, NEUTRAL, WORST }    // Q3: good, middle, bad
  }
};

// interview scoring is intentionally unequal.
// array order: Unethical, Buddy, Professional, Distracted, Pressuring.
// professional is most favorable; Buddy and Pressuring are tied;
// distracted is harder; unethical is the most punishing.
const int bestAnswerPoints[INTERVIEWER_TYPE_COUNT] = {
  2, 3, 5, 2, 3
};

const int neutralAnswerPoints[INTERVIEWER_TYPE_COUNT] = {
  -8, -5, -3, -9, -5
};

const int poorAnswerPoints[INTERVIEWER_TYPE_COUNT] = {
  -25, -20, -15, -25, -20
};

// IMPORTANT: final health must be over 50 even if you make it all the way
const int WIN_THRESHOLD = 50;

// hiring potential score
// converts the numerical health into text
const char* getHiringPotentialText(int potential) {
  if (potential > WIN_THRESHOLD) {
    return "Good";
  } 
  else if (potential <= 2) {
    return "Very Poor";
  } 
  else if (potential <= 17) {
    return "Poor";
  } 
  else {
    return "Fair";
  }
}

// store items helo a corresponding interviewer. they are capped
int getStoreBonus() {
  switch (currentInterviewer) {
    case DISTRACTED: return min(practice, 2) * 4;           // up to two practices help distracted
    case UNETHICAL: return min(childcare, 1) * 8;   // up to one childcare helps with unethical
    case PROFESSIONAL: return min(linkedin, 1) * 4;         // up to one linkedin helps professional
    case BUDDY: return min(latte, 2) * 4;                   // up to two lattes helps buddy
    default: return 0;                                      // pressuring has no matching store item.
  }
}



//-------------------------------SCREEN FUNCTIONS-----------------------------------

// title screen
void showTitle() {
  clearScreen();
  tft.textMode();
  tft.textColor(RA8875_GREEN, RA8875_BLACK);
  tft.textEnlarge(4);
  writeText(175, 150, "THE JOB SEARCH");
  tft.textEnlarge(0);
  writeText(310, 250, "Press [Enter] to begin");
  writeText(225, 300, "Press [1] to learn more about The Job Search");
  writeText(225, 320, "Press [2] for The Job Search research sources");
}

// learn more screen
void showLearnMore() {
  clearScreen();
  writeTitle(F("ABOUT THE JOB SEARCH"));
  writeText(20, 80, F("There are currently 7.1 million unemployed people in the United States."));
  writeText(20, 130, F("The Job Search takes users through the journey of interviewing for a job by depicting"));
  writeText(20, 150, F("the harshness unpredictability, and nonsense of the interview process. The Job Search"));
  writeText(20, 170, F("invites viewers to determine their background, advantages, and make choices in the"));
  writeText(20, 190, F("hopes of landing a job."));
  writeText(20, 390, F("Press [Enter] to return to the title screen"));
  finishScreen();
}

// sources screen
void showSources() {
  clearScreen();
  tft.textEnlarge(0);
  tft.textColor(RA8875_GREEN, RA8875_BLACK);

  writeTitle(F("SOURCES"));

  int y = 58;

  // Allwork.Space
  writeText(20, y, "Allwork.Space News Team. \"Job Seekers Report Spending Hundreds.\" Allwork.Space, Dec. 5, 2025.");
  y += 16;

  // CNBC
  writeText(20, y, "CNBC. \"Is an Ivy League Degree Worth It? Report Finds Advantages Beyond Pay.\" Aug. 2, 2023.");
  y += 16;

  // Duke University
  writeText(20, y, "Duke University, Fuqua School of Business. \"Research Finds Social-Class Bias Can Lead to");
  writeText(40, y + 16, "Hiring Discrimination.\" June 20, 2023.");
  y += 32;

  // Mimbela
  writeText(20, y, "Mimbela, Ricardo, and Olga Akselrod. \"The Long History of Discrimination in Job");
  writeText(40, y + 16, "Hiring Assessments.\" ACLU, May 30, 2024.");
  y += 32;

  // Milne
  writeText(20, y, "Milne, Stefan. \"People Mirror AI Systems' Hiring Biases.\" University of Washington News,");
  writeText(40, y + 16, "Nov. 10, 2025.");
  y += 32;

  // Minnesota Educational Computing Corporation
  writeText(20, y, "Minnesota Educational Computing Corporation (MECC). The Oregon Trail. Game manual.");
  writeText(40, y + 16, "Minneapolis: MECC, n.d.");
  y += 32;

  // NPR
  writeText(20, y, "NPR. \"Resume Bias Study: White Names, Black Names.\" Apr. 11, 2024.");
  y += 16;

  // Nuytten
  writeText(20, y, "Nuytten, Natasha. \"What the Data Says: The State of Hiring Bias in 2025.\" CultureCon,");
  writeText(40, y + 16, "Apr. 24, 2025.");
  y += 32;

  // Oregon Trail Archive
  writeText(20, y, "Oregon Trail Archive. \"The Oregon Trail.\" Accessed Nov. 15, 2026.");
  y += 16;

  // Travis
  writeText(20, y, "Travis, Michelle. \"How to Reduce Hiring Bias Against Women? Study Finds Key Role for HR.\"");
  writeText(40, y + 16, "Forbes, Feb. 11, 2025.");
  y += 32;

  // U.S. Bureau of Labor Statistics
  writeText(20, y, "U.S. Bureau of Labor Statistics. \"The Employment Situation. October 2026.\" Nov. 6, 2026.");
  writeText(20, 390, F("Press [Enter] to return to the title screen"));
  finishScreen();
}

// name choice screen
void showName() {
  clearScreen();
  writeTitle(F("CHOOSE YOUR NAME"));
  writeText(20, 75, F("1. Brad"));
  writeText(20, 110, F("2. Ebony"));
  writeText(20, 145, F("3. Lamar"));
  writeText(20, 180, F("4. Kirsten"));
  writeText(20, 245, F("Press [1], [2], [3], or [4] to select a name"));
  finishScreen();
}

// age choice comments
void showAge() {
  clearScreen();
  writeTitle(F("HOW OLD ARE YOU?"));
  writeText(20, 75, F("1. 20"));
  writeText(20, 110, F("2. 30"));
  writeText(20, 145, F("3. 40"));
  writeText(20, 180, F("4. 50"));
  writeText(20, 245, F("Press [1], [2], [3], or [4] to select an age"));
  finishScreen();
}

// school choice screen
void showSchool() {
  clearScreen();
  writeTitle(F("WHERE DID YOU STUDY?"));
  writeText(20, 75, F("1. Ivy League"));
  writeText(20, 110, F("2. State university"));
  writeText(20, 145, F("3. Community college"));
  writeText(20, 245, F("Press [1], [2] or [3] to select a name"));
  finishScreen();
}

//-------------------------------STORE-----------------------------------------------

// shop instructions screen
void shopIntro(){
  clearScreen();
  tft.graphicsMode();
  tft.fillScreen(RA8875_BLACK);

  // draw 300 x 150 image centered on 800 x 480 screen
  uint16_t rowBuffer[300];

  for (int y = 0; y < 150; y++) {
    for (int x = 0; x < 300; x++) {
      rowBuffer[x] = pgm_read_word(&prep[y * 300 + x]);
    }

    tft.drawPixels(rowBuffer, 300, 250, 165 + y);
  }

  tft.textMode();
  tft.textColor(RA8875_GREEN, RA8875_BLACK);

  writeTitle(F("PREP FOR THE INTERVIEW"));

  writeText(75, 75, F("Before setting out on your job search you should prepare for the interview process."));
  writeText(105, 110, F("Your credit card limit for this is $800, but you don't have to max it out."));

  writeText(315, 390, F("Press [Enter] to shop"));
  finishScreen();
}

// shopping screen
void showShop() {
  clearScreen();
  writeTitle(F("PREP FOR THE INTERVIEW"));

  writeText(20, 65, F("1. 1 on 1 practice interview sessions  $300")); // distracted
  writeText(20, 95, F("2. Childcare                           $600")); // helps with unethical; keeps you in if you pick you have kids
  writeText(20, 125, F("3. Year of LinkedIn Premium         $239.88")); // professional
  writeText(20, 155, F("4. Overpriced latte                     $30")); // the buddy

  sprintf(lineBuffer, "Practice sessions: %d", practice);
  writeText(430, 65, lineBuffer);
  sprintf(lineBuffer, "Childcare: %d", childcare);
  writeText(430, 95, lineBuffer);
  sprintf(lineBuffer, "Year of LinkedIn: %d", linkedin);
  writeText(430, 125, lineBuffer);
  sprintf(lineBuffer, "Overpriced latte: %d", latte);
  writeText(430, 155, lineBuffer);

  sprintf(lineBuffer, "Money left: $%.2f", money);
  writeText(20, 245, lineBuffer);

  writeText(20, 300, F("Press [1], [2], [3], and [4] to buy items. Press [Enter] to continue."));
  finishScreen();
}

// send off screen
void showReady() {
  clearScreen();

  tft.graphicsMode();
  tft.fillScreen(RA8875_BLACK);
  
  // draw 300 x 150 image centered on 800 x 480 screen
  uint16_t rowBuffer[300];

  for (int y = 0; y < 150; y++) {
    for (int x = 0; x < 300; x++) {
      rowBuffer[x] = pgm_read_word(&ready[y * 300 + x]);
    }

    tft.drawPixels(rowBuffer, 300, 250, 165 + y);
  }

  tft.textMode();
  tft.textColor(RA8875_GREEN, RA8875_BLACK);

  writeTitle(F("READY TO GO"));
  writeText(268, 75, F("Well then, you're ready to start."));
  writeText(132, 95, F("There will be three rounds of interviews with three questions each."));
  writeText(152, 115, F("You have a long and difficult journey ahead of you. Good luck!"));
  writeText(232, 390, F("Press [Enter] to start the first interview"));
  finishScreen();
}

//-------------------------------INTERVIEWS------------------------------------------

// randomly provide an interviewer
void chooseInterviewer() {
  int available[5];
  int count = 0;

  for (int i = 0; i < 5; i++) {
    if (!interviewerUsed[i]) {
      available[count] = i;
      count++;
    }
  }

  int selected = random(0, count);
  currentInterviewer = available[selected];

  interviewerUsed[currentInterviewer] = true;
  interviewStep = 0;
}

// draw the corresponding image of the interviewer
void drawInterviewerImage(const uint16_t *image) {
  uint16_t rowBuffer[300];

  for (int y = 0; y < 150; y++) {
    for (int x = 0; x < 300; x++) {
      rowBuffer[x] = pgm_read_word(&image[y * 300 + x]);
    }

    tft.drawPixels(rowBuffer, 300, 250, 165 + y);
  }
}

// interviewer intro screen
void showInterviewIntro() {
  tft.graphicsMode();
  tft.fillScreen(RA8875_BLACK);

  // draw the image corresponding to the interviewer
  switch (currentInterviewer) {
    case UNETHICAL:
      drawInterviewerImage(unethical);
      break;

    case BUDDY:
      drawInterviewerImage(buddy);
      break;

    case PROFESSIONAL:
      drawInterviewerImage(professional);
      break;

    case DISTRACTED:
      drawInterviewerImage(distracted);
      break;

    case PRESSURING:
      drawInterviewerImage(pressured);
      break;
  }

  tft.textMode();
  tft.textColor(RA8875_GREEN, RA8875_BLACK);
  tft.textEnlarge(1);
  tft.textSetCursor(20, 20);

  switch (currentInterviewer) {
    case UNETHICAL:
      tft.textWrite("Interviewer: HR Representative");
      break;

    case BUDDY:
      tft.textWrite("Interviewer: Direct Colleague");
      break;

    case PROFESSIONAL:
      tft.textWrite("Interviewer: Hiring Manager");
      break;

    case DISTRACTED:
      tft.textWrite("Interviewer: Department Manager");
      break;

    case PRESSURING:
      tft.textWrite("Interviewer: Executive");
      break;
  }

  tft.textEnlarge(0);
  tft.textSetCursor(20, 75);

  switch (currentInterviewer) {
    case UNETHICAL:
      tft.textWrite("They care about who you are!");
      break;

    case BUDDY:
      tft.textWrite("They want to make sure you feel welcome!");
      break;

    case PROFESSIONAL:
      tft.textWrite("They have a notebook and everything!");
      break;

    case DISTRACTED:
      tft.textWrite("Busy guy, still making time for you!");
      break;

    case PRESSURING:
      tft.textWrite("Straight to business!");
      break;
  }

  tft.textSetCursor(315, 390);
  tft.textWrite("PRESS [Enter] TO BEGIN");

  finishScreen();
}

// formatting helper function
// create a dashed line around the question
void drawFormattedQuestion(int x, int y, const char* question) {
  int len = strlen(question);

  // create dashed line matching question length
  char dashedLine[100];

  for (int i = 0; i < len && i < 99; i++) {
    dashedLine[i] = '-';
  }
  dashedLine[len < 99 ? len : 99] = '\0';

  // combine everything into one string
  char formattedQuestion[250];

  snprintf(formattedQuestion, sizeof(formattedQuestion),
           "%s\n%s\n%s",
           dashedLine, question, dashedLine);

  writeText(x, y, formattedQuestion);
}

// show the interview screen
void showInterview() {
  clearScreen();

  sprintf(lineBuffer, "INTERVIEW %d", interviewNumber);
  writeTitle(lineBuffer);

  sprintf(lineBuffer, "Current hiring potential: %s", getHiringPotentialText(health));
  writeText(20, 65, lineBuffer);

  sprintf(lineBuffer, "Question %d of 3", interviewStep + 1);
  writeText(20, 100, lineBuffer);

  drawFormattedQuestion(20, 145,
    interviewQuestions[currentInterviewer][interviewStep]);

  writeText(20, 220, "1.");
  writeText(55, 220,
    interviewAnswers[currentInterviewer][interviewStep][0]);

  writeText(20, 265, "2.");
  writeText(55, 265,
    interviewAnswers[currentInterviewer][interviewStep][1]);

  writeText(20, 310, "3.");
  writeText(55, 310,
    interviewAnswers[currentInterviewer][interviewStep][2]);

  writeText(20, 380, F("Press [1], [2], or [3] to answer"));
  finishScreen();
}

//-------------------------------CROSSINGS--------------------------------------------

// crossing intro screen
void showCrossingIntro() {
  tft.graphicsMode();
  tft.fillScreen(RA8875_BLACK);

  // Draw 300 x 150 image centered on 800 x 480 screen
  uint16_t rowBuffer[300];

  for (int y = 0; y < 150; y++) {
    for (int x = 0; x < 300; x++) {
      rowBuffer[x] = pgm_read_word(&final[y * 300 + x]);
    }

    tft.drawPixels(rowBuffer, 300, 250, 165 + y);
  }

  tft.textMode();
  tft.textColor(RA8875_GREEN, RA8875_BLACK);
  tft.textEnlarge(1);

  if (currentCrossing == FOLLOW_UP_EMAIL) {
    tft.textSetCursor(128, 72);
    tft.textWrite("YOU NEED TO SEND A FOLLOW UP EMAIL");
  } else {
    tft.textSetCursor(128, 72);
    tft.textWrite("TIME TO PREP FOR THE FINAL INTERVIEW");
  }

  tft.textEnlarge(0);
  tft.textSetCursor(300, 390);
  tft.textWrite("Press [Enter] to continue");

  finishScreen();
}

// crossing screen
void showCrossing() {
  clearScreen();

  if (currentCrossing == FOLLOW_UP_EMAIL) {
    writeTitle(F("FOLLOW-UP EMAIL"));

    writeText(20, 110, F("1. Send a follow up within 24 hours!"));
    // outcome 1: they are super interested
    // outcome 2: they are desperate

    writeText(20, 155, F("2. Send repeated check-ins."));
    // outcome 1: they are committed!
    // outcome 2: they are annoying!

    writeText(20, 200, F("3. Personalize and restate fit."));
    // safest option: neutral

    writeText(20, 245, F("4. Assume no reply means rejection."));
    // outcome 1: you were right, rejected
    // outcome 2: you were wrong, but now you didnt show you want it, rejected

  } else {
    writeTitle(F("FINAL INTERVIEW PREP"));

    writeText(20, 110, F("1. Practice and sleep."));
    // safest option: neutral outcome

    writeText(20, 155, F("2. Buy an expensive new outfit."));
    // best option: you look professional, employers definitely prioritize looks!

    writeText(20, 200, F("3. Research the interview panel."));
    // outcome 1: yay you did your research
    // outcome 2: awkward that you knew their dog's name

    writeText(20, 245, F("4. Cram all night."));
    // outcome 1: sacrifice pays off
    // outcome 2: you overslept and missed the interview
  }

  writeText(20, 300, F("Press [1], [2], [3] or [4] to make a choice"));
  finishScreen();
}

// outcome screen
void showOutcome() {
  clearScreen();

  tft.textEnlarge(1);
  writeText(20, 80, crossingOutcome);
  tft.textEnlarge(0);

  writeText(20, 150, F("Press [Enter] to continue"));

  finishScreen();
}

// apply the crossing choice 50/50 good or bad with few exceptions
void applyCrossingChoice(int choice) {
  lastHealthChange = 0;
  crossingOutcome = "";

  if (currentCrossing == FOLLOW_UP_EMAIL) {

    if (choice == 1) {
      if (random(0, 2) == 0) {
        crossingOutcome = "Showing interest!";
        lastHealthChange = 5;
      } else {
        crossingOutcome = "Oooh desperate";
        lastHealthChange = -15;
      }

    } else if (choice == 2) {
      if (random(0, 2) == 0) {
        crossingOutcome = "So committed!";
        lastHealthChange = 5;
      } else {
        crossingOutcome = "Kinda annoying no?";
        lastHealthChange = -15;
      }

    } else if (choice == 3) {
      crossingOutcome = "Okay, nice";
      lastHealthChange = 0;

    } else {
      if (random(0, 2) == 0) {
        crossingOutcome = "You were right, rejected!";
        lastHealthChange = -100;
      } else {
        crossingOutcome = "You were wrong, but no email? rejected!";
        lastHealthChange = -100;
      }
    }

  } else {

    if (choice == 1) {
      crossingOutcome = "Okay, nice!";
      lastHealthChange = 0;

    } else if (choice == 2) {
      crossingOutcome = "Employers appreicate expensive professionalism";
      lastHealthChange = 10;

    } else if (choice == 3) {
      if (random(0, 2) == 0) {
        crossingOutcome = "Nice, you did your research!";
        lastHealthChange = 5;
      } else {
        crossingOutcome = "Kinda awkward that you knew their dog's name";
        lastHealthChange = -20;
      }

    } else {
      if (random(0, 2) == 0) {
        crossingOutcome = "Sacrifice pays off!";
        lastHealthChange = 5;
      } else {
        crossingOutcome = "Bleary eyed and falling asleep..?";
        lastHealthChange = -20;
      }
    }
  }

  health += lastHealthChange;
  health = constrain(health, 0, MAX_HEALTH);

  state = EVENT_SCREEN;
  showOutcome();
}

//-------------------------------END SCREENS------------------------------------------

// show losing screen
void showLose() {
  clearScreen();
  tft.graphicsMode();
  tft.fillScreen(RA8875_BLACK);

  // draw 300 x 150 image centered on 800 x 480 screen
  uint16_t rowBuffer[300];

  for (int y = 0; y < 150; y++) {
    for (int x = 0; x < 300; x++) {
      rowBuffer[x] = pgm_read_word(&lose[y * 300 + x]);
    }

    tft.drawPixels(rowBuffer, 300, 250, 165 + y);
  }

  tft.textMode();
  tft.textColor(RA8875_GREEN, RA8875_BLACK);

  tft.textEnlarge(1);
  tft.textSetCursor(220, 60);
  tft.textWrite("YOU HAVE BEEN REJECTED");

  tft.textEnlarge(0);
  tft.textSetCursor(280, 125);
  tft.textWrite("Hiring potential fell to zero.");

  tft.textSetCursor(300, 390);
  tft.textWrite("Press [Enter] to see why");

  finishScreen();
}

// show win screen
void showWin() {
  clearScreen();
  tft.graphicsMode();
  tft.fillScreen(RA8875_BLACK);

  // draw 300 x 150 image centered on 800 x 480 screen
  uint16_t rowBuffer[300];

  for (int y = 0; y < 150; y++) {
    for (int x = 0; x < 300; x++) {
      rowBuffer[x] = pgm_read_word(&win[y * 300 + x]);
    }

    tft.drawPixels(rowBuffer, 300, 250, 165 + y);
  }

  tft.textMode();
  tft.textColor(RA8875_GREEN, RA8875_BLACK);

  tft.textEnlarge(1);
  tft.textSetCursor(280, 60);
  tft.textWrite("YOU GOT THE JOB");

  tft.textEnlarge(0);
  tft.textSetCursor(280, 125);
  tft.textWrite("You survived The Job Search.");

  tft.textSetCursor(300, 340);
  tft.textWrite("Congratulations, candidate.");

  tft.textSetCursor(300, 390);
  tft.textWrite("Press [Enter] to learn why");

  finishScreen();
}

// show the stats of the identity choices
void showStats() {
  clearScreen();
  writeTitle(F("IDENTITY AND BIAS"));
  sprintf(lineBuffer, "Name: %s", candidateName);
  writeText(20, 65, lineBuffer);

  if (strcmp(candidateName, "Brad") == 0 || strcmp(candidateName, "Kirsten") == 0) {
    writeText(20, 95, F("Traditionally white names can increase your chances in the interview process."));
  }
  if (strcmp(candidateName, "Lamar") == 0 || strcmp(candidateName, "Ebony") == 0) {
    writeText(20, 95, F("Traditionally non-white names can decrease your chances in the interview process."));
  }
  if (strcmp(candidateName, "Kirsten") == 0 || strcmp(candidateName, "Ebony") == 0) {
    writeText(20, 120, F("Women report experiencing gender bias and inappropriate questioning in interviews."));
  }

  sprintf(lineBuffer, "Age: %d", age);
  writeText(20, 165, lineBuffer);

  if (age == 20) {
    writeText(20, 195, F("Young generations can face interviewers questioning their professionalism."));
  } else if (age == 40 || age == 50) {
    writeText(20, 195, F("Older applicants face ageism because interviewers worry they'll leave soon or be"));
    writeText(20, 215, F("expensive employees to hire because of experience."));
  } else {
    writeText(20, 195, F("With some experience in the work force, you escape immediate bias toward incoming"));
    writeText(20, 215, F("professionals and late career professionals."));
  }

  if (schoolChoice == 1) {
    writeText(20, 255, F("School: Ivy League"));
    writeText(20, 285, F("This can double or triple your chances of being hired as a prestigious graduate."));
  } else if (schoolChoice == 2) {
    writeText(20, 255, F("School: State School"));
    writeText(20, 285, F("State school degrees can often be seen as less prestigous and rigorous compared to Ivys."));
  } else {
    writeText(20, 255, F("School: Community College"));
    writeText(20, 285, F("Community college degree are often equivalent to state schools as it shows dedication"));
    writeText(20, 305, F("towards edudcation."));
  }

  // writeText(20, 345, F("In an already unpredictable market, job bias still exists."));
  // writeText(20, 365, F("How did it change how you played?"));
  writeText(20, 390, F("Press [Enter] to learn how different personalities shaped your experience"));
  finishScreen();
}

// descriptions of the interviewers
const char* interviewerDescriptions[5] = {
  "The Unethical Interviewer: Asked discriminatory questions to determine shady company standards.",
  "The Friend Interviewer: Asked conversational questions to determine your lifestyle outside \nof the workplace.",
  "The Professional Interviewer: Focused on qualifications, experise, and proficiency.",
  "The Distracted Interviwer: Was barely paying attention and did not give you the time of day.",
  "The Pressuring Interviwer: Asked questions to push you into committing to a questionable \nworkplace environment."
};

// show the stats on who the interviewers were
void showStats2() {
  clearScreen();

  writeTitle(F("UNPREDICTABLE INTERVENTIONS"));

  for (int i = 0; i < 3; i++) {
    if (interviewerHistory[i] >= 0) {
      sprintf(lineBuffer, "ROUND %d:", i + 1);
      writeText(20, 80 + (i * 90), lineBuffer);

      writeText(20, 105 + (i * 90),
        interviewerDescriptions[interviewerHistory[i]]);
    }
  }

  writeText(20, 390, F("Press [Enter] to continue"));
  finishScreen();
}

// parting words screen
void end() {
  clearScreen();

  tft.textEnlarge(1);
  tft.textColor(RA8875_GREEN, RA8875_BLACK);

  tft.textSetCursor(120, 130);
  tft.textWrite("Realize it or not, the job search");

  tft.textSetCursor(125, 190);
  tft.textWrite("is full of prejudice, unfairness,");

  tft.textSetCursor(160, 250);
  tft.textWrite("and uncontrollable variables.");

  tft.textEnlarge(0);
  tft.textSetCursor(208, 390);
  tft.textWrite("Press [Enter] to submit your next application");
}

//-------------------------------CALCULATIONS------------------------------------------

// starting health based off of identity choices
void calculateStartingHealth() {
  health = 38;

  // name bias: Brad > Kirsten > Lamar > Ebony.
  if (nameChoice == 1) health += 12;
  else if (nameChoice == 4) health += 8;
  else if (nameChoice == 3) health += 2;
  else if (nameChoice == 2) health -= 4;

  // Age bias: 30 > 20 > 40 > 50.
  if (age == 30) health += 6;
  else if (age == 20) health += 2;
  else if (age == 40) health -= 2;
  else if (age == 50) health -= 6;

  // ivy League receives an advantage; state and community are tied.
  if (schoolChoice == 1) health += 6;

  health = constrain(health, 0, MAX_HEALTH);
}

void applyInterviewAnswer(int answer) {
  // immediate loss if you are pregnant unless you have childcare
  if (currentInterviewer == UNETHICAL &&
    interviewStep == 2 &&
    answer == 2 &&
    childcare == 0) {

  health = 0;
  state = LOSE_SCREEN;
  showLose();
  return;
}

  // look up the quality of this specific answer.
  int quality = answerQuality[currentInterviewer][interviewStep][answer - 1];

  int change = 0;

  if (quality == BEST) {
    change = bestAnswerPoints[currentInterviewer];
  } else if (quality == NEUTRAL) {
    change = neutralAnswerPoints[currentInterviewer];
  } else {
    change = poorAnswerPoints[currentInterviewer];
  }

  // only the item designed for this interviewer gives a bonus.
  change += getStoreBonus();

  health += change;
  health = constrain(health, 0, MAX_HEALTH);

  if (health <= 0) {
    state = LOSE_SCREEN;
    showLose();
    return;
  }

  interviewStep++;

  if (interviewStep >= 3) {
    if (interviewNumber == 1) {
      currentCrossing = FOLLOW_UP_EMAIL;
      state = CROSSING_INTRO;
      showCrossingIntro();
    } else if (interviewNumber == 2) {
      currentCrossing = FINAL_PREP;
      state = CROSSING_INTRO;
      showCrossingIntro();
    } else if (interviewNumber == 3) {
      if (health >= 50) {
        state = WIN_SCREEN;
        showWin();
      } else {
        state = LOSE_SCREEN;
        showLose();
      }
    }
  } else {
    showInterview();
  }
}

//-------------------------------GAME RESET-------------------------------------------

// reset all of the variables
void resetGame() {
  state = TITLE;

  candidateName = "Not selected";
  nameChoice = 0;
  age = 0;
  schoolChoice = 0;

  health = 0;
  practice = 0;
  childcare = 0;
  latte = 0;
  linkedin = 0;
  money = 800.00;

  interviewNumber = 1;
  interviewStep = 0;
  currentInterviewer = 0;
  currentCrossing = 0;
  lastHealthChange = 0;
  crossingOutcome = "";

  for (int i = 0; i < 5; i++) {
    interviewerUsed[i] = false;
  }
}

//-------------------------------GAME PLAY--------------------------------------------

// set up the system and variables
void setup() {
  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);
  pinMode(btn3, INPUT_PULLUP);
  pinMode(btn4, INPUT_PULLUP);
  pinMode(btn5, INPUT_PULLUP);
  pinMode(btn_enter, INPUT_PULLUP);

  Serial.begin(115200);

  // ESP32 VSPI: SCK=18, MISO=19, MOSI=23; CS is GPIO21.
  SPI.begin(18, 19, 23, RA8875_CS);
  if (!tft.begin(RA8875_800x480)) {
    Serial.println(F("RA8875 Not found!"));
    while (1) {}
  }

  tft.displayOn(true);
  tft.GPIOX(true);
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024);
  tft.PWM1out(255);

  randomSeed(analogRead(34));
  showTitle();
}

void loop() {
  // HOME / RESTART works from every screen, including the title screen.
  if (buttonPressed(btn5)) {
    resetGame();
    showTitle();
    return;
  }

  // STATE MACHINE FLOW OF GAME
  switch (state) {
    case TITLE:
      if (buttonPressed(btn1)) {
        state = LEARN_MORE;
        showLearnMore();
      } else if (buttonPressed(btn2)) {
        state = SOURCES;
        showSources();
      }
      else if (buttonPressed(btn_enter)) {
        state = CHOOSE_NAME;
        showName();
      }
      break;

    case SOURCES:
      if (buttonPressed(btn_enter)) {
        state = TITLE;
        showTitle();
      }
      break;

    case LEARN_MORE:
      if (buttonPressed(btn_enter)) {
        state = TITLE;
        showTitle();
      }
      break;

    case CHOOSE_NAME:
      if (buttonPressed(btn1)) {
        nameChoice = 1;
        candidateName = "Brad";
      } else if (buttonPressed(btn2)) {
        nameChoice = 2;
        candidateName = "Ebony";
      } else if (buttonPressed(btn3)) {
        nameChoice = 3;
        candidateName = "Lamar";
      } else if (buttonPressed(btn4)) {
        nameChoice = 4;
        candidateName = "Kirsten";
      }

      if (nameChoice > 0) {
        state = CHOOSE_AGE;
        showAge();
      }
      break;

    case CHOOSE_AGE:
      if (buttonPressed(btn1)) age = 20;
      else if (buttonPressed(btn2)) age = 30;
      else if (buttonPressed(btn3)) age = 40;
      else if (buttonPressed(btn4)) age = 50;

      if (age > 0) {
        state = CHOOSE_SCHOOL;
        showSchool();
      }
      break;

    case CHOOSE_SCHOOL:
      if (buttonPressed(btn1)) {
        schoolChoice = 1;
      } else if (buttonPressed(btn2)) {
        schoolChoice = 2;
      } else if (buttonPressed(btn3)) {
        schoolChoice = 3;
      }

      if (schoolChoice > 0) {
        calculateStartingHealth();
        state = SHOP_INTRO;
        shopIntro();
      }
      break;

    case SHOP_INTRO:
      if (buttonPressed(btn_enter)) {
        state = SHOP;
        showShop();
      }
      break;

    case SHOP:
      if (buttonPressed(btn1)) {
        if (money >= 300) {
          money -= 300;
          practice++;
        }
        showShop();
      } else if (buttonPressed(btn2)) {
        if (money >= 600) {
          money -= 600;
          childcare++;
        }
        showShop();
      } else if (buttonPressed(btn3)) {
        if (money >= 239.88) {
          money -= 239.88;
          linkedin++;
        }
        showShop();
      } else if (buttonPressed(btn4)) {
        if (money >= 30) {
          money -= 30;
          latte++;
        }
        showShop();
      } else if (buttonPressed(btn_enter)) {
        state = READY;
        showReady();
      }
      break;

    case READY:
      if (buttonPressed(btn_enter)) {
        interviewNumber = 1;
        interviewStep = 0;
        chooseInterviewer();
        interviewerHistory[interviewNumber - 1] = currentInterviewer;
        state = INTERVIEW_INTRO;
        showInterviewIntro();
      }
      break;

    case INTERVIEW_INTRO:
      if (buttonPressed(btn_enter)) {
        state = INTERVIEW;
        showInterview();
      }
      break;

    case INTERVIEW:
      if (buttonPressed(btn1)) {
        applyInterviewAnswer(1);
      } else if (buttonPressed(btn2)) {
        applyInterviewAnswer(2);
      } else if (buttonPressed(btn3)) {
        applyInterviewAnswer(3);
      }
      break;
    
    case CROSSING_INTRO:
      if (buttonPressed(btn_enter)) {
        state = CROSSING;
        showCrossing();
      }
      break;


    case CROSSING:
    if (buttonPressed(btn1)) {
      applyCrossingChoice(1);
    }
    else if (buttonPressed(btn2)) {
      applyCrossingChoice(2);
    }
    else if (buttonPressed(btn3)) {
      applyCrossingChoice(3);
    }
    else if (buttonPressed(btn4)) {
      applyCrossingChoice(4);
    }
  break;

    case EVENT_SCREEN:
    if (buttonPressed(btn_enter)) {
      if (health <= 0) {
        state = LOSE_SCREEN;
        showLose();
      } else {
        if (currentCrossing == FOLLOW_UP_EMAIL) {
          interviewNumber = 2;
        } else {
          interviewNumber = 3;
        }

        interviewStep = 0;
        chooseInterviewer();
        interviewerHistory[interviewNumber - 1] = currentInterviewer;
        state = INTERVIEW_INTRO;
        showInterviewIntro();
      }
    }
  break;

    case WIN_SCREEN:
    case LOSE_SCREEN:
      if (buttonPressed(btn_enter)) {
        state = STATS_SCREEN;
        showStats();
      }
      break;

    case STATS_SCREEN:
      if (buttonPressed(btn_enter)) {
        state = STATS_SCREEN2;
        showStats2();
      }
      break;

      case STATS_SCREEN2:
        if (buttonPressed(btn_enter)) {
        state = END;
        end();
      }
      break;

      case END:
        if (buttonPressed(btn_enter)) {
        resetGame();
        showTitle();
      }
      break;
  }
}
