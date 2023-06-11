import { useState } from 'react';
import './App.css';
import InputBox from './InputBox';
import TopBar from './TopBar';
import Conversation from './Conversation';
import ConversationStep from './ConversationStep';
import Intro from './Intro';
import AutoScrollWrapper from './AutoScrollWrapper';

const apiUrl = 'http://localhost:8080';
const defaultConversation = { title: 'Empty chat', steps: [] };

function App() {
  const [input, setInput] = useState("");
  const [question, setQuestion] = useState("");
  const [answer, setAnswer] = useState("");
  const [isBusy, setIsBusy] = useState(false);
  const [conversation, setConversation] = useState(defaultConversation);
  const [waiting, setWaiting] = useState(true);

  function getChatCompletionRequest(userPrompt) {
    const payload = {
      "model": "gpt-3.5-turbo",
      "messages": [{"role": "user", "content": userPrompt}],
      "stream": true
    }
    return JSON.stringify(payload)
  }
 
  function onResponseFinished(question, answer) {
    setConversation(prev => ({
      ...prev,
      steps: [...prev.steps, { question, answer }]
    }));
    setAnswer("");
    setQuestion("");
  }

  function parseServerSentEvents(data) {
    return data.split('\n').filter(line => line.startsWith("data:")).map(line => line.substring(5));
  }

  function onResponseUpdate(previous, newChunk) {
    let result = "";
    setWaiting(false);
    try {
      const events = parseServerSentEvents(newChunk);
      if (events) {
        events.forEach(ev => {
          const parsed = JSON.parse(ev);
          const c0 = parsed && parsed.choices && parsed.choices[0];
          if (!c0) return;
          const eventText = c0 && c0.delta && c0.delta.content;
          result += eventText;
        });
      }
      setAnswer(previous + result);
    } catch {
      console.log("Error parsing response")
    }
    return result;
  }

  async function handleSubmit() {
    setIsBusy(true);
    setQuestion(input);
    setAnswer("");
    setWaiting(true);
    setInput("");
    const payload = getChatCompletionRequest(input);
    try {
      const response = await fetch(apiUrl + '/v1/chat/completions', { method: "POST", body: payload });
      const reader = response.body.getReader();
      let partialResponse = "";
      while (true) {
        const { done, value } = await reader.read();
        if (done) {
          onResponseFinished(input, partialResponse);
          break;
        }
        const newChunk = new TextDecoder().decode(value);
        partialResponse += onResponseUpdate(partialResponse, newChunk);
      }
    } catch (err) {
      console.log("Error fetching:", err);
    }
    setIsBusy(false);
  }

  async function handleCancel() {
    // TODO: Check response
    await fetch(apiUrl + '/cancel');
    setIsBusy(false);
  }

  return (
    <AutoScrollWrapper>
      <div className="App">
        <TopBar title={conversation.title} />
        <div className='Container'>
          <Intro />
          <Conversation conversation={conversation} />
          {isBusy && <ConversationStep question={question} answer={answer} waiting={waiting} />}
          {isBusy && <button onClick={handleCancel} className="Button">Cancel generation</button>}
          <InputBox value={input} onChange={e => setInput(e.target.value)} onSubmit={handleSubmit} disabled={isBusy} />

        </div>
      </div>
    </AutoScrollWrapper>
  );
}

export default App;
