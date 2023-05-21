import { useState } from 'react';
import './App.css';
import InputBox from './InputBox';
import TopBar from './TopBar';
import Conversation from './Conversation';
import ConversationStep from './ConversationStep';
import Intro from './Intro';
import AutoScrollWrapper from './AutoScrollWrapper';

const endPoint = 'http://localhost:8080';
const defaultConversation = { title: 'Empty chat', steps: [] };

function App() {
  const [input, setInput] = useState("");
  const [question, setQuestion] = useState("");
  const [answer, setAnswer] = useState("");
  const [isBusy, setIsBusy] = useState(false);
  const [conversation, setConversation] = useState(defaultConversation);
  const [waiting, setWaiting] = useState(true);

  function onResponseFinished(question, answer) {
    setConversation(prev => ({
      ...prev,
      steps: [ ...prev.steps, {question, answer}]
    }));
    setAnswer("");
    setQuestion("");
  }

  function onResponseUpdate(previousChunk, newChunk) {
    setWaiting(false);
    return previousChunk + newChunk;
  }

  async function handleSubmit() {
    setIsBusy(true);
    setQuestion(input);
    setAnswer("...");
    setWaiting(true);
    setInput("");
    let tmpAnswer = "";
    const response = await fetch(endPoint + '/chat', { method: "POST", body: input });
    const reader = response.body.getReader();
    while (true) {
      const { done, value } = await reader.read();
      if (done) {
        onResponseFinished(input, tmpAnswer);
        break;
      }
      const newChunk = new TextDecoder().decode(value);
      tmpAnswer = onResponseUpdate(tmpAnswer, newChunk);
      setAnswer(tmpAnswer);
    }
    setIsBusy(false);
  }

  async function handleCancel() {
    // TODO: Check response
    await fetch(endPoint + '/cancel');
    setIsBusy(false);
  }

  return (
    <AutoScrollWrapper>
      <div className="App">
        <TopBar title={conversation.title} />
        <div className='Container'>
          <Intro />
          <Conversation conversation={conversation} />
          {isBusy && <ConversationStep question={question} answer={answer} waiting={waiting}/>}
          {isBusy && <button onClick={handleCancel} className="Button">Cancel generation</button> }
          <InputBox value={input} onChange={e => setInput(e.target.value)} onSubmit={handleSubmit} disabled={isBusy} />
          
        </div>
      </div>
    </AutoScrollWrapper>
  );
}

export default App;
